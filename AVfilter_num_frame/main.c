#include <stdio.h>
#include <stdlib.h>

#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
#include "libavfilter/version.h"

int process_avfilter(FILE *fp_in,FILE *fp_out,int in_width,int in_height,const char *filter_descr)
{
    int ret=0;
    AVFrame *frame_in;
    AVFrame *frame_out;
    unsigned char *frame_buffer_in;
    unsigned char *frame_buffer_out;

    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;

    avfilter_register_all();

    char args[10000];

    //获取要使用的过滤器
    //过滤器链的开始
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");

    //过滤器链的结束
    AVFilter *buffersink = avfilter_get_by_name("buffersink");

    //buffersrc的出口
    AVFilterInOut *outputs = avfilter_inout_alloc();

    //buffersink的入口
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    enum AVPixelFormat pix_fmts[]={AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVBufferSinkParams *buffersink_params;

    //创建AVFilterGraph
    filter_graph = avfilter_graph_alloc();

    snprintf(args, sizeof(args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        in_width,in_height,AV_PIX_FMT_YUV420P,
        1, 15,1,1);

    //由buffersrc创建buffersrc_ctx，并将参数传给buffersrc_ctx，之后将buffersrc_ctx传给filter_graph
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
        args, NULL, filter_graph);
//    printf("ret1:%d\n",&ret);
    if (ret < 0) {
        printf("Cannot create buffer source\n");
        return ret;
    }

    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
//    printf(buffersink_params->pixel_fmts);
    //创建并向FilterGraph中添加一个Filter
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
        NULL, buffersink_params, filter_graph);

    av_free(buffersink_params);
//    printf("ret2:%d\n",&ret);

    if (ret < 0) {
        printf("Cannot create buffer sink\n");
        return ret;
    }

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    //连接过滤器，并把水印参数和输入输出参数添加到filter_graph中
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr,
        &inputs, &outputs, NULL)) < 0)
        return ret;

    //检查config设置是否正确
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        return ret;

    frame_in=av_frame_alloc();
    frame_buffer_in=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, in_width,in_height,1));
    av_image_fill_arrays(frame_in->data, frame_in->linesize,frame_buffer_in,
        AV_PIX_FMT_YUV420P,in_width, in_height,1);

    frame_out=av_frame_alloc();
    frame_buffer_out=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, in_width,in_height,1));
    av_image_fill_arrays(frame_out->data, frame_out->linesize,frame_buffer_out,
        AV_PIX_FMT_YUV420P,in_width, in_height,1);

    frame_in->width=in_width;
    frame_in->height=in_height;
    frame_in->format=AV_PIX_FMT_YUV420P;
    int num_frame=1;

    while (1) {

        if(fread(frame_buffer_in, 1, in_width*in_height*3/2, fp_in)!= in_width*in_height*3/2){
            break;
        }
        //输入
        frame_in->data[0]=frame_buffer_in;                             // Y
        frame_in->data[1]=frame_buffer_in+in_width*in_height;          // U
        frame_in->data[2]=frame_buffer_in+in_width*in_height*5/4;      // V

        //向过滤器链中添加一个视频帧
        if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0) {
            printf( "添加帧错误.\n");
            break;
        }

        //从过滤器链中取出一个处理好的视频帧
        ret = av_buffersink_get_frame(buffersink_ctx, frame_out);
        if (ret < 0)
            break;

        //输出
        if(frame_out->format==AV_PIX_FMT_YUV420P){
            for(int i=0;i<frame_out->height;i++){
                fwrite(frame_out->data[0]+frame_out->linesize[0]*i,1,frame_out->width,fp_out);      //Y
            }
            for(int i=0;i<frame_out->height/2;i++){
                fwrite(frame_out->data[1]+frame_out->linesize[1]*i,1,frame_out->width/2,fp_out);    //U
            }
            for(int i=0;i<frame_out->height/2;i++){
                fwrite(frame_out->data[2]+frame_out->linesize[2]*i,1,frame_out->width/2,fp_out);    //V
            }
        }
        printf("成功处理视频帧：%d\n",num_frame);
        num_frame++;
        av_frame_unref(frame_out);
    }

    fclose(fp_in);
    fclose(fp_out);

    av_frame_free(&frame_in);
    av_frame_free(&frame_out);
    avfilter_graph_free(&filter_graph);

    return 0;
}

int main()
{
    //源yuv
    FILE *fp_in=fopen("./test.yuv","rb+");
    if(fp_in==NULL){
        printf("Error open input file.\n");
        return -1;
    }
    int in_width=720;
    int in_height=1280;

    //目标yuv
    FILE *fp_out=fopen("./output.yuv","wb+");
    if(fp_out==NULL){
        printf("Error open output file.\n");
        return -1;
    }

    //水印描述,这里是为每帧yuv添加帧编号
    const char *filter_descr = "drawtext=fontfile=/Library/Fonts/SimHei.ttf:fontcolor=red:shadowy=2:fontsize=60:text=%{n}:x=10:y=10";

    process_avfilter(fp_in,fp_out,in_width,in_height,filter_descr);

    return 0;
}
