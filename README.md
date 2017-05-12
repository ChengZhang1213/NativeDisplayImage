# NativeDisplayImage
使用 OpenGLES2 在Native展示图片

#概述
替代在Java层调用OpenGLES2方式，直接在Native层使用C/C++方式调用。
本Demo没有使用Android自身的BitmapFactory API 来读取图片，而是使用OpenCV来读取的图片，需要注意将图片
的格式转换，转换代码如下：
```
   IplImage *src = cvLoadImage(imgPath);
   IplImage *dst = cvCreateImage(cvGetSize(src),src->depth,src->nChannels);
   //you should confirm the image type
   cvCvtColor(src,dst,CV_BGR2RGB);
```
之后需要在生成文理的时候确定纹理的类型,一般为GL_RGB、GLRGBA，如下：
 ```
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
 ```
在Native层不同与Java层，glTexImage2D传入对象为Pixel数据.

本例子使用了VBO的方式加载纹理，你也可以将顶点坐标和文理坐标分开定义，但是根据OpenGLES开放者建议使用VBO

#TODO
加入特征点到提取和显示，添加SIFT的方式，参考于[sift-gpu-iphone][1]
但是算出的结果和IOS的结果不一致 T_T


  [1]: https://github.com/Moodstocks/sift-gpu-iphone
