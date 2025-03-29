#include "glad.h"
#include <GLFW/glfw3.h>
#include <GL/glut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "shader_m.h"
#include "shader_c.h"

#include <iostream>
#include <unistd.h>
#include <filesystem>
#include <vector>
#include <map>
#include <set>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
int dX = 0,dY = 0;
int resolution = 10;

// texture size
const unsigned int TEXTURE_WIDTH = 512*(1+.5*(resolution+1)), TEXTURE_HEIGHT = 512*(1+.5*(resolution+1));
unsigned int texture[2];

float Max(float a,float b) {
	return (a<b)?b:a;
}

struct Pattern {
	std::string name;
	int ni;
	int nj;
	float *** dat;
	Pattern(std::string _name,int step=0,int padding=5):name(_name),dat(NULL),ni(-1),nj(-1) {
		GLint imageWidth,imageHeight,components;
		unsigned char* data;
		std::cout << "reading: " << name << std::endl;
		data = stbi_load(name.c_str(),&imageWidth,&imageHeight,&components,STBI_rgb_alpha);
		ni = imageWidth+2*padding;
		nj = imageHeight+2*padding;
		if(ni != nj) {
			std::cout << "please provide square images" << std::endl;
			exit(1);
		}
		std::cout << ni << "x" << nj << std::endl;
		dat = new float**[3];
		dat[0] = new float*[imageWidth+2*padding];
		dat[1] = new float*[imageWidth+2*padding];
		dat[2] = new float*[imageWidth+2*padding];
		for(int x=0;x<imageWidth+2*padding;x++) {
			dat[0][x] = new float[imageHeight+2*padding];
			dat[1][x] = new float[imageHeight+2*padding];
			dat[2][x] = new float[imageHeight+2*padding];
			for(int y=0;y<imageHeight+2*padding;y++) {
				if(x>=padding&&x<imageWidth+padding&&y>=padding&&y<imageHeight+padding) {
					dat[0][x][y] = (float)data[4*(imageWidth*(y-padding)+(x-padding))  ]/255;
					dat[1][x][y] = (float)data[4*(imageWidth*(y-padding)+(x-padding))+1]/255;
					dat[2][x][y] = (float)data[4*(imageWidth*(y-padding)+(x-padding))+2]/255;
				} else {
					dat[0][x][y] = 0;
					dat[1][x][y] = 0;
					dat[2][x][y] = 0;
				}
				if(dat[0][x][y] < 0.5) dat[0][x][y] = 0;
				if(dat[0][x][y] > 0.5) dat[0][x][y] = 1;
			}
		}
		delete [] data;
		for(int i=0;i<step;i++) {
			simulate(dat[0],ni,nj);
		}
	}
	void simulate(float ** data,int nx,int ny) {
		float ** orig = new float*[nx];
		for(int x=0;x<nx;x++) {
			orig[x] = new float[ny];
			for(int y=0;y<ny;y++) {
				orig[x][y] = data[x][y];
			}
		}
		for(int x=1;x+1<nx;x++) {
			for(int y=1;y+1<ny;y++) {
				float num = 0.0;
				float r = orig[x][y];
				for(int i=-1;i<=1;i++) {
					for(int j=-1;j<=1;j++) {
						num += orig[x+i][y+j];
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
				data[x][y] = r;
			}
		}
	}
	void apply(int x,int y,int wx,int wy,float * data) const {
		for(int i=0;i<ni;i++) {
			for(int j=0;j<nj;j++) {
				data[4*((x+j)*wy+(y+i))  ] = Max(data[4*((x+j)*wy+(y+i))  ],dat[0][j][i]);
				data[4*((x+j)*wy+(y+i))+1] = Max(data[4*((x+j)*wy+(y+i))+1],dat[1][j][i]);
				data[4*((x+j)*wy+(y+i))+2] = Max(data[4*((x+j)*wy+(y+i))+2],dat[2][j][i]);
				data[4*((x+j)*wy+(y+i))+3] = 1;
			}
		}
	}
};

namespace fs = std::filesystem;

std::vector<std::string> get_all_files_in_directory(const std::string& path) {
	std::vector<std::string> files;
	for (const auto& entry : fs::directory_iterator(path)) {
		if (fs::is_regular_file(entry)) {
			//files.push_back(entry.path().string());
			files.push_back(entry.path().filename().string());
		}
	}
	return files;
}

int countSubstrings(const std::string& text, const std::string& pattern) {
	int count = 0;
	size_t position = 0;
	while ((position = text.find(pattern, position)) != std::string::npos) {
		++count;
		position += pattern.length(); // Move past the found occurrence
	}
	return count;
}

std::vector<std::string> split(const std::string& text, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream ss(text);
	std::string token;
	while (std::getline(ss, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

struct PatternCollection {
	std::vector<int> index;
	std::vector<int> max_index;
	std::map<int,std::map<int,std::map<int,std::map<int,std::map<int,Pattern*> > > > > pattern;
	void load(std::string collection,int step=1) {
		std::stringstream ss;
		ss << "patterns/" << collection << "/";
		auto files = get_all_files_in_directory(ss.str());
		int layers = 0;
		int _layers = -1;
		for(auto x : files) {
			if(x.find("pattern") != std::string::npos) {
				std::cout << x << std::endl;
				layers = countSubstrings(x,"-");
				if(_layers != -1 && layers != _layers) {
					std::cout << "layers don't match" << std::endl;
					exit(1);
				}
				_layers = layers;
			}
		}
		std::vector<std::set<int>> labels(5);
		for(int i=0;i<5-layers;i++) labels[i].insert(0);
		for(int i=0;i<step;i++) labels[0].insert(i);
		for(auto x : files) {
			if(x.find("pattern") != std::string::npos) {
				std::vector<std::string> tokens = split(x,'-');
				if(tokens.empty()) {
					std::cout << "need - to split" << std::endl;
					exit(1);
				}
				for(int i=1;i<tokens.size();i++) {
					auto t = split(tokens[i],'.');
					std::cout << t[0] << std::endl;
					labels[layers+i].insert(atoi(t[0].c_str()));
				}
			}
		}
		for(int i=0;i<labels.size();i++) {
			std::cout << labels[i].size() << std::endl;
		}
		for(int i0=0;i0<labels[0].size();i0++)
		for(int i1=0;i1<labels[1].size();i1++)
		for(int i2=0;i2<labels[2].size();i2++)
		for(int i3=0;i3<labels[3].size();i3++)
		for(int i4=0;i4<labels[4].size();i4++) {
			std::stringstream ss;
			ss << "patterns/" << collection << "/pattern";
			if(layers>4)ss << "-" << i0;
			if(layers>3)ss << "-" << i1;
			if(layers>2)ss << "-" << i2;
			if(layers>1)ss << "-" << i3;
			if(layers>0)ss << "-" << i4;
			ss << ".png";
			pattern[i0][i1][i2][i3][i4] = new Pattern(ss.str(),i0);
			std::cout << pattern[i0][i1][i2][i3][i4]->name << std::endl;
		}
		for(int i=0;i<labels.size();i++)index.push_back(0);
		for(int i=0;i<labels.size();i++)max_index.push_back(labels[i].size());
	}
	Pattern * get() {
		Pattern * tmp = pattern[index[0]][index[1]][index[2]][index[3]][index[4]];
		std::cout << tmp->name << std::endl;
		return tmp;
	}
	Pattern * get(int i0,int i1,int i2,int i3,int i4) {
		Pattern * tmp = pattern[i0][i1][i2][i3][i4];
		return tmp;
	}
	void up(int id) {
		index[id]++;
		index[id]=index[id]%max_index[id];
	}
	void down(int id) {
		index[id]--;
		index[id]=(index[id]+100*max_index[id])%max_index[id];
	}
};

struct Stopper {
	const int ni=4;
	const int nj=4;
	float dat[4][4] = {
	{1,1,0,0},
	{1,0,0,0},
	{0,1,1,1},
	{0,0,0,1}
	};
	void apply(int x,int y,int wx,int wy,float * out,int rotation=1,int layer=0) const {
		switch(rotation) {
			case 1: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+j)*wy+(y+i))+layer] = dat[j][i];
					}
				}
				break;
			}
			case 2: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+i)*wy+(y+(nj-1-j)))+layer] = dat[j][i];
					}
				}
				break;
			}
			case 3: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+(nj-1-j))*wy+(y+(ni-1-i)))+layer] = dat[j][i];
					}
				}
				break;
			}
			case 4: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+(ni-1-i))*wy+(y+j))+layer] = dat[j][i];
					}
				}
				break;
			}
			case -1: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+j)*wy+(y+i))+layer] = dat[j][ni-1-i];
					}
				}
				break;
			}
			case -2: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+i)*wy+(y+(nj-1-j)))+layer] = dat[j][ni-1-i];
					}
				}
				break;
			}
			case -3: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+(nj-1-j))*wy+(y+(ni-1-i)))+layer] = dat[j][ni-1-i];
					}
				}
				break;
			}
			case -4: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+(ni-1-i))*wy+(y+j))+layer] = dat[j][ni-1-i];
					}
				}
				break;
			}
		}
	}
};

struct Glider {
	const int ni=3;
	const int nj=3;
	float dat[3][3] = {
	{1,0,0},
	{0,1,1},
	{1,1,0}
	};
	void apply(int x,int y,int wx,int wy,float * out,int rotation=1,int layer=0) const {
		switch(rotation) {
			case 1: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+j)*wy+(y+i))+layer] = dat[j][i];
					}
				}
				break;
			}
			case 2: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+i)*wy+(y+(nj-1-j)))+layer] = dat[j][i];
					}
				}
				break;
			}
			case 3: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+(nj-1-j))*wy+(y+(ni-1-i)))+layer] = dat[j][i];
					}
				}
				break;
			}
			case 4: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+(ni-1-i))*wy+(y+j))+layer] = dat[j][i];
					}
				}
				break;
			}
			case -1: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+j)*wy+(y+i))+layer] = dat[j][ni-1-i];
					}
				}
				break;
			}
			case -2: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+i)*wy+(y+(nj-1-j)))+layer] = dat[j][ni-1-i];
					}
				}
				break;
			}
			case -3: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+(nj-1-j))*wy+(y+(ni-1-i)))+layer] = dat[j][ni-1-i];
					}
				}
				break;
			}
			case -4: {
				for(int i=0;i<ni;i++) {
					for(int j=0;j<nj;j++) {
						out[4*((x+(ni-1-i))*wy+(y+j))+layer] = dat[j][ni-1-i];
					}
				}
				break;
			}
		}
	}
};

struct GosperGun {
	float dat[9][36] = {
	{0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,1, 0,0,0,0,0,  0,0,0,0,0, 0},
	{0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,  0,0,1,0,1, 0,0,0,0,0,  0,0,0,0,0, 0},
	{0,0,0,0,0, 0,0,0,0,0,  0,0,1,1,0, 0,0,0,0,0,  1,1,0,0,0, 0,0,0,0,0,  0,0,0,0,1, 1},
	{0,0,0,0,0, 0,0,0,0,0,  0,1,0,0,0, 1,0,0,0,0,  1,1,0,0,0, 0,0,0,0,0,  0,0,0,0,1, 1},
	{1,1,0,0,0, 0,0,0,0,0,  1,0,0,0,0, 0,1,0,0,0,  1,1,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0},
	{1,1,0,0,0, 0,0,0,0,0,  1,0,0,0,1, 0,1,1,0,0,  0,0,1,0,1, 0,0,0,0,0,  0,0,0,0,0, 0},
	{0,0,0,0,0, 0,0,0,0,0,  1,0,0,0,0, 0,1,0,0,0,  0,0,0,0,1, 0,0,0,0,0,  0,0,0,0,0, 0},
	{0,0,0,0,0, 0,0,0,0,0,  0,1,0,0,0, 1,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0},
	{0,0,0,0,0, 0,0,0,0,0,  0,0,1,1,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0},
	};
	int stopper = 0;
	void apply(int x,int y,int wx,int wy,float * out,int rotation=1) {
		switch(rotation) {
			case 1: {
				for(int i=0;i<36;i++) {
					for(int j=0;j<9;j++) {
						out[4*((x+j)*wy+(y+i))] = dat[j][i];
						out[4*((x+j)*wy+(y+i))+2] = dat[j][i];
					}
				}
				break;
			}
			case 2: {
				for(int i=0;i<36;i++) {
					for(int j=0;j<9;j++) {
						out[4*((x+i)*wy+(y+(8-j)))] = dat[j][i];
						out[4*((x+i)*wy+(y+(8-j)))+2] = dat[j][i];
					}
				}
				break;
			}
			case 3: {
				for(int i=0;i<36;i++) {
					for(int j=0;j<9;j++) {
						out[4*((x+(8-j))*wy+(y+(35-i)))] = dat[j][i];
						out[4*((x+(8-j))*wy+(y+(35-i)))+2] = dat[j][i];
					}
				}
				if(stopper > 0) {
					Stopper s;
					int i=stopper;
					s.apply(x+20-25-i,y+20-11-i,wx,wy,out,3);
				}
				Glider g;
				int d = stopper;
				if(stopper<=0)d=100;
				for(int i=0;15*i<d;i++) {
					g.apply(x+23-26-15*i,y-15*i+10,wx,wy,out,3,2);
					g.apply(x+22-26-7-15*i,y+10-7-15*i,wx,wy,out,-2,2);
					if(i>0)g.apply(x+23-25-15*i,y+6-15*i,wx,wy,out,-3,1);
				}
				break;
			}
			case 4: {
				for(int i=0;i<36;i++) {
					for(int j=0;j<9;j++) {
						out[4*((x+(35-i))*wy+(y+j))] = dat[j][i];
						out[4*((x+(35-i))*wy+(y+j))+2] = dat[j][i];
					}
				}
				if(stopper > 0) {
					Stopper s;
					int i=stopper;
					s.apply(x+20-10-i,y+20-11+i,wx,wy,out,4);
				}
				Glider g;
				int d = stopper;
				if(stopper<=0)d=100;
				for(int i=0;15*i<d;i++) {
					g.apply(x+23-13-15*i,y+15*i+9,wx,wy,out,4,2);
					g.apply(x+22-11-8-15*i,y+9+8+15*i,wx,wy,out,-3,2);
					if(i>0)g.apply(x+23-6-13-15*i,y+1+15*i+9,wx,wy,out,-4,1);
				}
				break;
			}
			case -1: {
				for(int i=0;i<36;i++) {
					for(int j=0;j<9;j++) {
						out[4*((x+j)*wy+(y+i))] = dat[j][35-i];
						out[4*((x+j)*wy+(y+i))+2] = dat[j][35-i];
					}
				}
				break;
			}
			case -2: {
				for(int i=0;i<36;i++) {
					for(int j=0;j<9;j++) {
						out[4*((x+i)*wy+(y+(8-j)))] = dat[j][35-i];
						out[4*((x+i)*wy+(y+(8-j)))+2] = dat[j][35-i];
					}
				}
				break;
			}
			case -3: {
				for(int i=0;i<36;i++) {
					for(int j=0;j<9;j++) {
						out[4*((x+(8-j))*wy+(y+(35-i)))] = dat[j][35-i];
						out[4*((x+(8-j))*wy+(y+(35-i)))+2] = dat[j][35-i];
					}
				}
				if(stopper > 0) {
					Stopper s;
					int i=stopper;
					s.apply(x+20-26-i,y+20+4+i,wx,wy,out,-3);
				}
				Glider g;
				int d = stopper;
				if(stopper<=0)d=100;
				for(int i=0;15*i<d;i++) {
					g.apply(x+23-26-15*i,y+15*i+23,wx,wy,out,-3,2);
					g.apply(x+22-26-7-15*i,y+23+7+15*i,wx,wy,out,4,2);
					if(i>0)g.apply(x+23-27-15*i,y+15*i+27,wx,wy,out,3,1);
				}
				break;
			}
			case -4: {
				for(int i=0;i<36;i++) {
					for(int j=0;j<9;j++) {
						out[4*((x+(35-i))*wy+(y+j))] = dat[j][35-i];
						out[4*((x+(35-i))*wy+(y+j))+2] = dat[j][35-i];
					}
				}
				if(stopper > 0) {
					Stopper s;
					int i=stopper;
					s.apply(x+20+3+i,y+20-10+i,wx,wy,out,-4);
				}
				Glider g;
				int d = stopper;
				if(stopper<=0)d=100;
				for(int i=0;15*i<d;i++) {
					g.apply(x+23+15*i,y+15*i+9,wx,wy,out,-4,2);
					g.apply(x+22+8+15*i,y+9+8+15*i,wx,wy,out,1,2);
					if(i>0)g.apply(x+6+23+15*i,y-1+15*i+9,wx,wy,out,4,1);
				}
				break;
			}
		}
	}
};

float DX = 0;
float DY = 0;
float DZ = 0;


struct PatternInfo {
	int id;
	int i0,i1,i2,i3,i4;
	int dX,dY;
};

struct Palette {
	std::vector<PatternInfo> history;
	std::vector<PatternInfo> history_redo;
	std::vector<PatternCollection*> collections;
	void add(std::string name,int step=1) {
		PatternCollection * pattern_collection = new PatternCollection();
		pattern_collection->load(name,step);
		collections.push_back(pattern_collection);
	}
	int id;
	void up() {
		id++;
		id=(id%collections.size());
	}
	void down() {
		id--;
		id=(id+100*collections.size())%collections.size();
	}
	PatternCollection * get() const {
		return collections[id];
	}
	void save() {
		history_redo.clear();
		history.push_back(PatternInfo{id,get()->index[0],get()->index[1],get()->index[2],get()->index[3],get()->index[4],dX,dY});
	}
	void undo() {
		if(history.size()>0) {
			history_redo.push_back(history[history.size()-1]);
			history.erase(history.begin()+(history.size()-1));
		}
	}
	void redo() {
		if(history_redo.size()>0) {
			history.push_back(history_redo[history_redo.size()-1]);
			history_redo.erase(history_redo.begin()+(history_redo.size()-1));
		}
	}
};

void init(Palette * palette,float * data) {
	for(int i=0;i<TEXTURE_WIDTH*TEXTURE_HEIGHT;i++) {
		data[4*i+0] = 0.0;
		data[4*i+1] = 0.0;
		data[4*i+2] = 0.0;
		data[4*i+3] = 1.0;
	}
	for(auto p : palette->history) {
		palette->collections[p.id]->get(p.i0,p.i1,p.i2,p.i3,p.i4)->apply(TEXTURE_WIDTH/2+p.dX,TEXTURE_HEIGHT/2+p.dY,TEXTURE_WIDTH,TEXTURE_HEIGHT,data);
	}
	palette->get()->get()->apply(TEXTURE_WIDTH/2+dX,TEXTURE_HEIGHT/2+dY,TEXTURE_WIDTH,TEXTURE_HEIGHT,data);
	//GosperGun g;
	//g.apply(TEXTURE_WIDTH/2,TEXTURE_HEIGHT/2,TEXTURE_WIDTH,TEXTURE_HEIGHT,data,-3);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, data);
	usleep(10000);
}

int main(int argc, char* argv[])
{
	Palette * palette = new Palette();
	palette->add("Gosper_gun");
	palette->add("stopper");
	palette->add("and");
	palette->add("or");
	palette->add("xor");
	palette->add("nor");
	palette->add("not");
	palette->add("reflector");
	palette->add("trail");
	palette->add("60p_gun",60);
	palette->add("buckaroo",30);
	palette->add("gliderduplicator1");
	palette->add("bifurcation");
	//palette->add("adder");
	//palette->add("metapixel");
	srand(time(0));
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Circuit", glfwGetPrimaryMonitor(), NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSwapInterval(0);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// query limitations
	// -----------------
	int max_compute_work_group_count[3];
	int max_compute_work_group_size[3];
	int max_compute_work_group_invocations;

	for (int idx = 0; idx < 3; idx++) {
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, idx, &max_compute_work_group_count[idx]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, idx, &max_compute_work_group_size[idx]);
	}	
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_compute_work_group_invocations);

	std::cout << "OpenGL Limitations: " << std::endl;
	std::cout << "maximum number of work groups in X dimension " << max_compute_work_group_count[0] << std::endl;
	std::cout << "maximum number of work groups in Y dimension " << max_compute_work_group_count[1] << std::endl;
	std::cout << "maximum number of work groups in Z dimension " << max_compute_work_group_count[2] << std::endl;

	std::cout << "maximum size of a work group in X dimension " << max_compute_work_group_size[0] << std::endl;
	std::cout << "maximum size of a work group in Y dimension " << max_compute_work_group_size[1] << std::endl;
	std::cout << "maximum size of a work group in Z dimension " << max_compute_work_group_size[2] << std::endl;

	std::cout << "Number of invocations in a single local work group that may be dispatched to a compute shader " << max_compute_work_group_invocations << std::endl;


	unsigned int DELTA_X = 8;
	unsigned int DELTA_Y = max_compute_work_group_size[0]/DELTA_X;
	for(;;) {
		if(DELTA_Y<=DELTA_X) {
			break;
		}
		DELTA_X += 8;
		DELTA_Y = max_compute_work_group_size[0]/DELTA_X;
	}
	unsigned int WORK_GROUP_X = (unsigned int)TEXTURE_WIDTH /DELTA_X + (TEXTURE_WIDTH %DELTA_X>0);
	unsigned int WORK_GROUP_Y = (unsigned int)TEXTURE_HEIGHT/DELTA_Y + (TEXTURE_HEIGHT%DELTA_Y>0);
	std::cout << DELTA_X << "x" << DELTA_Y << std::endl;


	// build and compile shaders
	// -------------------------
	Shader screenQuad("shaders/screenQuad.vs", "shaders/screenQuad.fs");
	ComputeShader update0Shader("shaders/update0.cs",DELTA_X,DELTA_Y);
	ComputeShader update1Shader("shaders/update1.cs",DELTA_X,DELTA_Y);

	screenQuad.use();

	float * data = new float[4*TEXTURE_WIDTH*TEXTURE_HEIGHT];
	for(int i=0;i<TEXTURE_WIDTH*TEXTURE_HEIGHT;i++) {
		data[4*i+0] = 0.0;
		data[4*i+1] = 0.0;
		data[4*i+2] = 0.0;
		data[4*i+3] = 1.0;
	}

	// Create texture for opengl operation
	// -----------------------------------
	glGenTextures(2, texture);
	for(int i=0;i<2;i++) {
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_2D, texture[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, data);
		glBindImageTexture(i, texture[i], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	}

	float * data1 = new float[4*TEXTURE_WIDTH*TEXTURE_HEIGHT];
	unsigned char * data2 = new unsigned char[3*TEXTURE_WIDTH*TEXTURE_HEIGHT];
	unsigned char * data3 = new unsigned char[3*TEXTURE_WIDTH*TEXTURE_HEIGHT];

	int speed = 1;
	int mode = 0;
	int delta = 1;
	// render loop
	// -----------
	glEnable(GL_DEPTH_TEST);
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window))
	{
		if(glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
			palette->up();
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
			palette->down();
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
			palette->save();
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
			palette->undo();
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
			palette->redo();
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
			palette->get()->up(0);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
			palette->get()->down(0);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
			palette->get()->up(1);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
			palette->get()->down(1);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
			palette->get()->up(2);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
			palette->get()->down(2);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
			palette->get()->up(3);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
			palette->get()->down(3);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) {
			palette->get()->up(4);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
			palette->get()->down(4);
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		{
			dX+=delta;
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		{
			dX-=delta;
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		{
			dY-=delta;
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		{
			dY+=delta;
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
		{
			delta/=2;
			if(delta<1)delta=1;
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
		{
			delta*=2;
			init(palette,data);
		}
		if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data1);
			int sx = TEXTURE_WIDTH *.5-TEXTURE_WIDTH *.5*DX-TEXTURE_WIDTH *.5*(1+DZ);
			int sy = TEXTURE_HEIGHT*.5-TEXTURE_HEIGHT*.5*DY-TEXTURE_HEIGHT*.5*(1+DZ);
			int bx = TEXTURE_WIDTH *.5-TEXTURE_WIDTH *.5*DX+TEXTURE_WIDTH *.5*(1+DZ);
			int by = TEXTURE_HEIGHT*.5-TEXTURE_HEIGHT*.5*DY+TEXTURE_HEIGHT*.5*(1+DZ);
			int _TEXTURE_WIDTH  = bx-sx;
			int _TEXTURE_HEIGHT = by-sy;
			if(_TEXTURE_WIDTH != _TEXTURE_HEIGHT) {
				int max_dimension = (_TEXTURE_WIDTH<_TEXTURE_HEIGHT)?_TEXTURE_HEIGHT:_TEXTURE_WIDTH;
				_TEXTURE_WIDTH  = max_dimension;
				_TEXTURE_HEIGHT = max_dimension;
			}
			for(int x=sx,_x=0;x<bx;x++,_x++) {
				for(int y=sy,_y=0;y<by;y++,_y++) {
					data2[3*(_TEXTURE_WIDTH*(_TEXTURE_HEIGHT-1-_y)+(_x))  ] = int(data1[4*(TEXTURE_WIDTH*y+x)  ]*255.99);
					data2[3*(_TEXTURE_WIDTH*(_TEXTURE_HEIGHT-1-_y)+(_x))+1] = int(data1[4*(TEXTURE_WIDTH*y+x)+1]*255.99);
					data2[3*(_TEXTURE_WIDTH*(_TEXTURE_HEIGHT-1-_y)+(_x))+2] = int(data1[4*(TEXTURE_WIDTH*y+x)+2]*255.99);
				}
			}
			std::string name = "Gosper_gun_2";
			{
				int mirror = 0;
				{
					std::stringstream ss;
					int rotation = 0;
					ss << "patterns/" << name << "/pattern" << "-" << mirror << "-" << rotation << ".png";
					stbi_write_png(ss.str().c_str(),_TEXTURE_WIDTH,_TEXTURE_HEIGHT,3,data2,_TEXTURE_WIDTH*3);
				}
				{
					std::stringstream ss;
					int rotation = 1;
					for(int _x=0;_x<_TEXTURE_WIDTH;_x++) {
						for(int _y=0;_y<_TEXTURE_HEIGHT;_y++) {
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))  ] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_WIDTH-1-_x)+(_y))  ];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+1] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_WIDTH-1-_x)+(_y))+1];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+2] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_WIDTH-1-_x)+(_y))+2];
						}
					}
					ss << "patterns/" << name << "/pattern" << "-" << mirror << "-" << rotation << ".png";
					stbi_write_png(ss.str().c_str(),_TEXTURE_WIDTH,_TEXTURE_HEIGHT,3,data3,_TEXTURE_WIDTH*3);
				}
				{
					std::stringstream ss;
					int rotation = 2;
					for(int _x=0;_x<_TEXTURE_WIDTH;_x++) {
						for(int _y=0;_y<_TEXTURE_HEIGHT;_y++) {
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))  ] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_HEIGHT-1-_y)+(_TEXTURE_WIDTH-1-_x))  ];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+1] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_HEIGHT-1-_y)+(_TEXTURE_WIDTH-1-_x))+1];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+2] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_HEIGHT-1-_y)+(_TEXTURE_WIDTH-1-_x))+2];
						}
					}
					ss << "patterns/" << name << "/pattern" << "-" << mirror << "-" << rotation << ".png";
					stbi_write_png(ss.str().c_str(),_TEXTURE_WIDTH,_TEXTURE_HEIGHT,3,data3,_TEXTURE_WIDTH*3);
				}
				{
					std::stringstream ss;
					int rotation = 3;
					for(int _x=0;_x<_TEXTURE_WIDTH;_x++) {
						for(int _y=0;_y<_TEXTURE_HEIGHT;_y++) {
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))  ] = data2[3*(_TEXTURE_WIDTH*(_x)+(_TEXTURE_HEIGHT-1-_y))  ];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+1] = data2[3*(_TEXTURE_WIDTH*(_x)+(_TEXTURE_HEIGHT-1-_y))+1];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+2] = data2[3*(_TEXTURE_WIDTH*(_x)+(_TEXTURE_HEIGHT-1-_y))+2];
						}
					}
					ss << "patterns/" << name << "/pattern" << "-" << mirror << "-" << rotation << ".png";
					stbi_write_png(ss.str().c_str(),_TEXTURE_WIDTH,_TEXTURE_HEIGHT,3,data3,_TEXTURE_WIDTH*3);
				}
			}
			{
				int mirror = 1;
				{
					std::stringstream ss;
					int rotation = 0;
					for(int _x=0;_x<_TEXTURE_WIDTH;_x++) {
						for(int _y=0;_y<_TEXTURE_HEIGHT;_y++) {
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))  ] = data2[3*(_TEXTURE_WIDTH*(_y)+(_x))  ];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+1] = data2[3*(_TEXTURE_WIDTH*(_y)+(_x))+1];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+2] = data2[3*(_TEXTURE_WIDTH*(_y)+(_x))+2];
						}
					}
					ss << "patterns/" << name << "/pattern" << "-" << mirror << "-" << rotation << ".png";
					stbi_write_png(ss.str().c_str(),_TEXTURE_WIDTH,_TEXTURE_HEIGHT,3,data3,_TEXTURE_WIDTH*3);
				}
				{
					std::stringstream ss;
					int rotation = 1;
					for(int _x=0;_x<_TEXTURE_WIDTH;_x++) {
						for(int _y=0;_y<_TEXTURE_HEIGHT;_y++) {
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))  ] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_WIDTH-1-_x)+(_y))  ];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+1] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_WIDTH-1-_x)+(_y))+1];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+2] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_WIDTH-1-_x)+(_y))+2];
						}
					}
					ss << "patterns/" << name << "/pattern" << "-" << mirror << "-" << rotation << ".png";
					stbi_write_png(ss.str().c_str(),_TEXTURE_WIDTH,_TEXTURE_HEIGHT,3,data3,_TEXTURE_WIDTH*3);
				}
				{
					std::stringstream ss;
					int rotation = 2;
					for(int _x=0;_x<_TEXTURE_WIDTH;_x++) {
						for(int _y=0;_y<_TEXTURE_HEIGHT;_y++) {
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))  ] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_HEIGHT-1-_y)+(_TEXTURE_WIDTH-1-_x))  ];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+1] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_HEIGHT-1-_y)+(_TEXTURE_WIDTH-1-_x))+1];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+2] = data2[3*(_TEXTURE_WIDTH*(_TEXTURE_HEIGHT-1-_y)+(_TEXTURE_WIDTH-1-_x))+2];
						}
					}
					ss << "patterns/" << name << "/pattern" << "-" << mirror << "-" << rotation << ".png";
					stbi_write_png(ss.str().c_str(),_TEXTURE_WIDTH,_TEXTURE_HEIGHT,3,data3,_TEXTURE_WIDTH*3);
				}
				{
					std::stringstream ss;
					int rotation = 3;
					for(int _x=0;_x<_TEXTURE_WIDTH;_x++) {
						for(int _y=0;_y<_TEXTURE_HEIGHT;_y++) {
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))  ] = data2[3*(_TEXTURE_WIDTH*(_x)+(_TEXTURE_HEIGHT-1-_y))  ];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+1] = data2[3*(_TEXTURE_WIDTH*(_x)+(_TEXTURE_HEIGHT-1-_y))+1];
							data3[3*(_TEXTURE_WIDTH*(_y)+(_x))+2] = data2[3*(_TEXTURE_WIDTH*(_x)+(_TEXTURE_HEIGHT-1-_y))+2];
						}
					}
					ss << "patterns/" << name << "/pattern" << "-" << mirror << "-" << rotation << ".png";
					stbi_write_png(ss.str().c_str(),_TEXTURE_WIDTH,_TEXTURE_HEIGHT,3,data3,_TEXTURE_WIDTH*3);
				}
			}
			std::cout << "screen captured" << std::endl;
		}
		if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
			DZ += .01;
			if(DZ>0)DZ=0;
		}
		if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			DZ -= .01;
			if(DZ<-.9)DZ=-.9;
		}
		if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			DY += .01;
		}
		if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			DY -= .01;
		}
		if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			DX += .01;
		}
		if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			DX -= .01;
		}
		if(glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		{
			speed*=2;
		}
		if(glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		{
			speed/=2;
			if(speed < 1) speed = 1;
		}

		update0Shader.use();
		glDispatchCompute(WORK_GROUP_X, WORK_GROUP_Y, 1);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		update1Shader.use();
		glDispatchCompute(WORK_GROUP_X, WORK_GROUP_Y, 1);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		// render image to quad
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		screenQuad.use();
		screenQuad.setInt("tex", 1);
		screenQuad.setVec3("offset", DX,DY,DZ);
		renderQuad();

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
		usleep(speed-1);
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteTextures(2, texture);
	glDeleteProgram(screenQuad.ID);

	glfwTerminate();

	return EXIT_SUCCESS;
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{

	if (quadVAO == 0)
	{
		float * quadVertices = new float[30];

		quadVertices[0] = -1.0f;
		quadVertices[1] = +1.0f;
		quadVertices[2] = 0.99f;
		quadVertices[3] = 0.0f;
		quadVertices[4] = 1.0f;

		quadVertices[5] = -1.0f;
		quadVertices[6] = -1.0f;
		quadVertices[7] = 0.99f;
		quadVertices[8] = 0.0f;
		quadVertices[9] = 0.0f;

		quadVertices[10] = +1.0f;
		quadVertices[11] = +1.0f;
		quadVertices[12] = 0.99f;
		quadVertices[13] = 1.0f;
		quadVertices[14] = 1.0f;

		quadVertices[15] = -1.0f;
		quadVertices[16] = -1.0f;
		quadVertices[17] = 0.99f;
		quadVertices[18] = 0.0f;
		quadVertices[19] = 0.0f;

		quadVertices[20] = +1.0f;
		quadVertices[21] = +1.0f;
		quadVertices[22] = 0.99f;
		quadVertices[23] = 1.0f;
		quadVertices[24] = 1.0f;

		quadVertices[25] = +1.0f;
		quadVertices[26] = -1.0f;
		quadVertices[27] = 0.99f;
		quadVertices[28] = 1.0f;
		quadVertices[29] = 0.0f;

		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(float), &quadVertices[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 30);
	glBindVertexArray(0);


}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

