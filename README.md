make sure your graphics drivers are up to date
on linux, you can run nvidia-smi, it should give you something like

```
nvidia-smi   
+-----------------------------------------------------------------------------------------+
| NVIDIA-SMI 560.35.03              Driver Version: 560.35.03      CUDA Version: 12.6     |
|-----------------------------------------+------------------------+----------------------+
| GPU  Name                 Persistence-M | Bus-Id          Disp.A | Volatile Uncorr. ECC |
| Fan  Temp   Perf          Pwr:Usage/Cap |           Memory-Usage | GPU-Util  Compute M. |
|                                         |                        |               MIG M. |
|=========================================+========================+======================|
|   0  NVIDIA GeForce GTX 1080        Off |   00000000:01:00.0  On |                  N/A |
| 29%   44C    P8             10W /  180W |     266MiB /   8192MiB |      5%      Default |
|                                         |                        |                  N/A |
+-----------------------------------------+------------------------+----------------------+
                                                                                         
+-----------------------------------------------------------------------------------------+
| Processes:                                                                              |
|  GPU   GI   CI        PID   Type   Process name                              GPU Memory |
|        ID   ID                                                               Usage      |
|=========================================================================================|
|    0   N/A  N/A      1808      G   /usr/lib/xorg/Xorg                            155MiB |
|    0   N/A  N/A      1965      G   /usr/bin/gnome-shell                           45MiB |
+-----------------------------------------------------------------------------------------+
```

if you don't have nvidia graphics card, it might still work, i have tested it with intel graphics card, it worked but was slow. 
might work on windows, you would have to figure out cygwin or visual studio though. 
mac os, definitely won't work, this code uses compute shaders, which are from OpenGL 4.3, mac os doesn't support anything above OpenGL 4.1, I'm too lazy to figure out a vulkan implemenation that runs on mac os as well. 

then type make

```
make
g++ -O3 -g0 glad.c main.cpp -lglfw -I. -o shader_test
```

it should generate a file ./shader_test

run it in terminal

useful controls:

A,W,S,D - more around

Q,Z - zoom in, zoom out

1,2,3,4,5,6,7,8,9,0 - cycle patterns

1,2 - tranditionally to cycle phase of pattern, which is important for some glider creators and reflectors

9,0 - rotate

7,8 - flip, change pattern, etc...

3,4,5,6 - up to you, just add some png files to patterns directory, only red component actually gets simulated, green, blue are for comments, follow file name convention, ex: pattern-I-J-K-L.png, note that all indices are in range [0,N) 

I,J,K,L - drag pattern around

T,Y - change drag step size

O,P - speed up, slow down simulation

ENTER - place pattern

N,M - unto, redo placement

SPACE - supposed to make a png file from current screen, might not work - work in progress, probably don't mess with it for now

ESC - exit

Warning: might seg fault, still debugging :-) 

