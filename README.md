#AVFilter_num_frame

为每一帧yuv添加数字水印，帧编号从0到最后一帧，也可更改drawtext部分添加其他水印。

开发环境：QT

编程库：ffmpeg

编程语言：C

添加水印的大致流程：

解码后的画面（源YUV）-- 加载 --> buffer过滤器 --> 其他过滤器 --> buffersink过滤器 -- 读取出来 --> 处理完的画面

结构类型：

AVFilterGraph：管理所有过滤器系统

AVFilterContext：过滤器上下文

AVFilter：过滤器


代码解读：

（一）创建过滤器链：

1.创建AVFilterGraph

AVFilterGraph *filter_graph=avfilter_graph_alloc();

2.获取要使用的过滤器

AVFilter *filter_buffer=avfilter_get_by_name("buffer");

AVFilter *filter_buffersink=avfilter_get_by_name("buffersink");

3.创建过滤器上下文

AVFilterContext *filter_buffer_ctx,*filter_buffersink_ctx;

avfilter_graph_create_filter(&filter_buffer_ctx,avfilter_get_by_name("buffer"),"in",args,NULL,filter_graph);

参数说明：

filter_buffer_ctx:用来保存创建好的过滤器上下文；

avfilter_get_by_name():过滤器

"in":过滤器名

args：传给过滤器的参数

filter_graph：过滤器图像管理指针


4.将filter和输入输出传给filter_graph

avfilter_garph_parse_ptr(filter_graph,filter_descr,&input,&output,NULL);

5.检查所有配置是否正确

if((ret=avfilter_graph_config(filter_graph,NULL))<0)

{

...

}

（二）使用过滤链进行过滤

1.将解码后的inframe推送给过滤器链

av_buffersrc_add_frame_filter(buffer_ctx,in_frame);

2.将处理完的frame拉取出来

av_buffersink_get_frame(filter_buffer_cink_ctx,out_frame);
