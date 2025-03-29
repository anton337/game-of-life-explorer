layout(rgba32f, binding = 0) uniform writeonly image2D display0;
layout(rgba32f, binding = 1) uniform readonly image2D display1;

void main(){
    ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
    float num = 0.0;
    vec3 p = imageLoad(display1, uv).rgb;
    float r = p.r;
    float g = p.g;
    float b = p.b;
    for(int i=-1;i<=1;i++) {
      for(int j=-1;j<=1;j++) {
        num += imageLoad(display1, ivec2(uv.x+i,uv.y+j)).r;
      }
    }
    num -= r;
    if(r > 0.5) {
      if(num < 1.5) {
        r = 0.0;
      }
      if(num > 3.5) {
        r = 0.0;
      }
    } else {
      if(num > 2.5 && num < 3.5) {
        r = 1.0;
      }
    }
    if(g<0.01&&uv.x%30==0)g=0.1;
    if(g<0.01&&uv.y%30==0)g=0.1;
    if(b<0.01&&g<0.01&&r>0.5)b=0.5;
    imageStore(display0, uv, vec4(r,g,b,1.0f));
}
