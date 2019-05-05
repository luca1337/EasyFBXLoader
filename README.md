# EasyFBXLoader -  External Tool

you can manage fbx with pure ease, no need to take coffee while loading anymore.

An easy fbx loader which converts all fbx in a new smart format for animations and mesh loading.

-To include in your project you must have fbxsdk from **Autodesk** and zlib in order to compress/decompress.

-it's a dinamyc link library so just put your dll inside your *.exe folder and don't forget to include **EfbxParser.h** 
inside your project where you need it.

-your output files will be two:

1) ".mesh" file which contains all related mesh datas for instance: vertices, normals, bones etc..
2) ".anim" file which contains all related mesh's bone frame in case you want to animate your character

At the first start it might take a while to create all output file that's because autodesk's sdk needs to do some process before building mesh, after the first start the EFL will check if the fbx file is already parsed or not, this will prevent file duplication.
