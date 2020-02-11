#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>

struct mesh{
  std::string meshName;
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> texturecoordinates;
  std::vector<glm::vec3> vertexnormals;
};

struct indexSet {
  int iv;
  int ivt;
  int ivn;
};

size_t linenumber = 0;

const std::set<std::string> supportedtokens{
  "v","vt","f","o","vn","#"
};
const std::set<std::string> ignoredtokens{
  "mtllib","s","usemtl",
};
const std::set<std::string> unsupportedtokens{
  "vp","cstype","deg","bmat","step","p","l","curv",
  "curv2","surf","parm","trim","hole","scrv","sp",
  "end","con","g","s","mg","bevel","c_interp",
  "d_interp","lod","usemtl","mtllib","shadow_obj",
  "trace_obj","ctech","stech","call","scmp","csh"
};

std::string tailstr(std::istringstream& tail){
  return tail.rdbuf()->str();
}
bool isempty(std::istringstream& tail){
  return tail.rdbuf()->in_avail() == 0;
}
void empty(std::istringstream& tail){
  if(!isempty(tail)){
    throw "Extra characters at the end of the line: '" + tailstr(tail) + "'";
  }
}
inline bool check(std::istream& is){
  if(!is){
    return false;
  }
  return true;
}
inline void pass(std::istream& is,  std::string s){
  if(!check(is)){
    throw s;
  }
}
inline void fail(std::istream& is, std::string s){
  if(check(is)){
    throw s;
  }
}

std::string parse_o(std::istringstream& tail){
  std::string name;
  pass((tail >> name),"No object name specified"); 
  empty(tail);
  return name;
}

glm::vec3 parse_v(std::istringstream& tail){
  float x,y,z,w;
  pass((tail >> x >> y >> z),"Invalid Vertex line format");
  fail((tail >> w),"w vertex parameter not supported");
  empty(tail);
  return {x,y,z};
}

glm::vec2 parse_vt(std::istringstream& tail){
  float a,b;
  pass((tail >> a >> b),"Invalid Vertex Texture line format"); 
  empty(tail);
  return {a,b};
}

glm::vec3 parse_vn(std::istringstream& tail){
  float x,y,z;
  pass((tail >> x >> y >> z),"Invalid Vertex Texture line format"); 
  empty(tail);
  return {x,y,z};
}

enum f_type {
  none,
  v,
  v_vt,
  v_vn,
  v_vt_vn
};

void slash(char slash){
  if(slash != '/'){
    throw "f dividing character should be a forward slash instead was a: " + std::string(1,slash);
  }
}

bool ftype_v(std::string fstr, int& iv){
  std::istringstream fstrstream(fstr);
  if(check((fstrstream >> iv))){
    if(isempty(fstrstream)){
      return true;
    }
  } 
  return false;
}

bool ftype_v_vt(std::string fstr, int& iv, int& ivt){
  char div = '\0';
  std::istringstream fstrstream(fstr);
  if(check((fstrstream >> iv >> div >> ivt))){
    if(isempty(fstrstream)){
      slash(div);
      return true;
    }
  }
  return false;
}

bool ftype_v_vn(std::string fstr, int& iv, int& ivn){
  char div[2] = {'\0','\0'};
  std::istringstream fstrstream(fstr);
  if(check((fstrstream >> iv >> div[0] >> div[1] >> ivn))){
    if(isempty(fstrstream)){
      slash(div[0]);
      slash(div[1]);
      return true;
    }
  }
  return false;
}

bool ftype_v_vt_vn(std::string fstr, int& iv, int& ivt, int& ivn){
  char div[2] = {'\0','\0'};
  std::istringstream fstrstream(fstr);
  if(check((fstrstream >> iv >> div[0] >> ivt >> div[1] >> ivn))){
    slash(div[0]);
    slash(div[1]);
    empty(fstrstream);
    return true;
  }
  return false;
}

f_type ftype(std::string fstr, int& iv, int& ivt, int& ivn){
  if(ftype_v(fstr,iv)){
    return v;
  }else if(ftype_v_vt(fstr,iv,ivt)){
    return v_vt;
  }else if(ftype_v_vn(fstr,iv,ivn)){
    return v_vn;
  }else if(ftype_v_vt_vn(fstr,iv,ivt,ivn)){
    return v_vt_vn;
  }
  throw "Invalid f mode";
}

void checkzeroes(f_type fmode, int iv, int ivt, int ivn){
  if(iv == 0 || (ivt == 0 && fmode != v_vn) || (ivn == 0 && fmode != v_vt)){
    throw "Indexing starts at 1";
  }
}

std::vector<indexSet> parse_f(std::istringstream& tail, f_type& fmode){
  std::string fstr;
  std::vector<indexSet> pairs;

  int iv;
  int ivt;
  int ivn;
  tail >> fstr;
  fmode = ftype(fstr,iv,ivt,ivn);
  checkzeroes(fmode,iv,ivt,ivn);
  pairs.push_back({iv-1,ivt-1,ivn-1});
  while((tail >> fstr)){
    std::istringstream fstrstream(fstr);
    char slashes[2] = {'/','/'};
    switch(fmode){
      case v:
        pass((fstrstream >> iv),"Invalid v mode");
        break;
      case v_vt:
        pass((fstrstream >> iv >> slashes[0] >> ivt),"Invalid v_vt mode");
        break;
      case v_vn:
        pass((fstrstream >> iv >> slashes[0] >> slashes[1] >> ivn),"Invalid v_vn mode");
        break;
      case v_vt_vn:
        pass((fstrstream >> iv >> slashes[0] >> ivt >> slashes[1] >> ivn),"Invalid v_vt_vn mode");
        break;
      case none:
        throw "Invalid f mode";
        break;
    }
    slash(slashes[0]);
    slash(slashes[1]);
    empty(fstrstream);
    checkzeroes(fmode,iv,ivt,ivn);
    pairs.push_back({iv-1,ivt-1,ivn-1});
  }
  if(pairs.size() < 3){
    throw "f must have at least 3 elements";
  }
  return pairs;
}

bool getfullline(std::istream& in, std::string& line){
  std::string buffer;
  if(!std::getline(in, buffer)){
    return false;
  }
  linenumber++;
  while(buffer.ends_with('\\')){
    buffer.pop_back();
    std::string addbuffer;
    if(!std::getline(std::cin, addbuffer)){
      throw "eof reached while combining lines";
    }
    linenumber++;
    buffer += addbuffer;
  }
  line = buffer;
  return true;
}


size_t getindex(long maxIndex, long pos){
  if(pos > maxIndex){
    throw "Index greater than maxIndex: " + std::to_string(pos) + " > " + std::to_string(maxIndex);
  }
  if(pos >= 0){
    return pos-1;
  }
  if(maxIndex - pos >= 0){
    return maxIndex-pos;
  }
  throw "Index references a vertex beyond 0";
}

std::vector<mesh> loadObj(std::istream& in){
  std::string line;
  std::vector<glm::vec3> iverts;
  std::vector<glm::vec2> itexs;
  std::vector<glm::vec3> inorms;
  std::vector<mesh> model;
  while (getfullline(in,line)){
    std::istringstream iss(line);
    std::string linetoken;
    if(!(iss >> linetoken)){ 
      break; 
    }
    if(ignoredtokens.contains(linetoken)){
      std::cout << "Warning: Unsupported line token: " + linetoken + " but possible to continue." << std::endl;
    } else if(unsupportedtokens.contains(linetoken)){
      throw "Unsupported line token: " + linetoken;
    } else if(!supportedtokens.contains(linetoken)){
      throw "Unexpected line token: " + linetoken;
    }
    if(linetoken == "o"){
      model.push_back({});
      model.back().meshName = parse_o(iss);
    } else if(linetoken == "v"){
      iverts.push_back(parse_v(iss));
    } else if(linetoken == "vt"){
      itexs.push_back(parse_vt(iss));
    } else if(linetoken == "vn"){
      inorms.push_back(parse_vn(iss));
    } else if(linetoken == "f"){
      f_type fmode;
      std::vector<indexSet> pairs = parse_f(iss, fmode);
      size_t ivertsize = iverts.size();
      size_t itexsize = itexs.size();
      size_t inormsize = inorms.size();
      for(size_t i = 0; i < pairs.size()-2; i++){
        model.back().vertices.insert(end(model.back().vertices),{
          iverts[getindex(ivertsize,pairs[0].iv)],
          iverts[getindex(ivertsize,pairs[i+1].iv)],
          iverts[getindex(ivertsize,pairs[i+2].iv)]
        });
        if(fmode == v_vt || fmode == v_vt_vn){
          model.back().texturecoordinates.insert(end(model.back().texturecoordinates),{
            itexs[getindex(itexsize,pairs[0].ivt)],
            itexs[getindex(itexsize,pairs[i+1].ivt)],
            itexs[getindex(itexsize,pairs[i+2].ivt)]
          });
        }
        if(fmode == v_vn || fmode == v_vt_vn){
          model.back().vertexnormals.insert(end(model.back().vertexnormals),{
            inorms[getindex(inormsize,pairs[0].ivn)],
            inorms[getindex(inormsize,pairs[i+1].ivn)],
            inorms[getindex(inormsize,pairs[i+2].ivn)]
          });
        }
      }
    }
  }
  return model;
}

int main(int argc, char** argv){
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
  std::vector<mesh> model;
  if(argc > 2){
    if(argc > 1){
      std::fstream obj(argv[1], std::ios::in);
      if(obj.is_open()){
        model = loadObj(obj);
      }else{
        throw "Failed to open file";
      }
    }else{
      model = loadObj(std::cin);
    }
  }else {
    try{
      if(argc > 1){
        std::fstream obj(argv[1], std::ios::in);
        if(obj.is_open()){
          model = loadObj(obj);
        }else{
          throw "Failed to open file";
        }
      }else{
        model = loadObj(std::cin);
      }
    }catch(const char* s){
      std::cout << "At line: " << linenumber << ": " << s << std::endl;
      return 1;
    }catch(std::string s){
      std::cout << "At line: " << linenumber << ": " << s << std::endl;
      return 1; 
    }
  }
  for(const auto & m : model){
    std::cout << m.meshName << std::endl;
    std::cout << m.texturecoordinates.size() << std::endl;
    std::cout << m.vertexnormals.size() << std::endl;
    std::cout << m.vertices.size() << std::endl;
  }
#endif
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
  while (__AFL_LOOP(10000)) {
    size_t linenumber = 0;
    try{
      for(const auto & m : loadObj(std::cin)){
        std::cout << m.meshName << std::endl;
        std::cout << m.texturecoordinates.size() << std::endl;
        std::cout << m.vertexnormals.size() << std::endl;
        std::cout << m.vertices.size() << std::endl;
      }
    }catch(const char* s){
      std::cout << "At line: " << linenumber << ": " << s << std::endl;
      return 1;
    }catch(std::string s){
      std::cout << "At line: " << linenumber << ": " << s << std::endl;
      return 1; 
    }
  }
#endif
  return 0;
}
