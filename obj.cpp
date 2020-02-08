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

bool empty(std::istringstream& tail){
  return tail.rdbuf()->in_avail() == 0;
}
std::string tailstr(std::istringstream& tail){
  return tail.rdbuf()->str();
}

std::string parse_o(std::istringstream& tail){
  std::string name;
  if (!(tail >> name)) {
     throw "No object name specified"; 
  }
  if(!empty(tail)){
    throw "Extra characters at the end of the line: '" + tailstr(tail) + "'";
  }
  return name;
}

glm::vec3 parse_v(std::istringstream& tail){
  float x,y,z,w;
  if (!(tail >> x >> y >> z)) {
     throw "Invalid Vertex line format"; 
  }
  if((tail >> w)) { 
    throw "w vertex parameter not supported";
  }
  if(!empty(tail)){
    throw "Extra characters at the end of the line: '" + tailstr(tail) + "'";
  }
  return {x,y,z};
}

glm::vec2 parse_vt(std::istringstream& tail){
  float a,b;
  if (!(tail >> a >> b)) {
     throw "Invalid Vertex Texture line format"; 
  }
  if(!empty(tail)){
    throw "Extra characters at the end of the line: '" + tailstr(tail) + "'";
  }
  return {a,b};
}

glm::vec3 parse_vn(std::istringstream& tail){
  float x,y,z;
  if (!(tail >> x >> y >> z)) {
     throw "Invalid Vertex Texture line format"; 
  }
  if(!empty(tail)){
    throw "Extra characters at the end of the line: '" + tailstr(tail) + "'";
  }
  return {x,y,z};
}

struct indices {
  int iv;
  int ivt;
  int ivn;
};
//in progess
std::vector<indices> parse_f(std::istringstream& tail){
  std::string fstr;
  std::vector<indices> pairs;
  while((tail >> fstr)){
    std::istringstream fstrstream(fstr);
    char slash;
    int iv,ivt,ivn;
    if(!(fstrstream >> iv >> slash >> ivt >> slash >> ivn)){ throw "unimplemented"; }
    pairs.push_back({iv-1,ivt-1,ivn-1});
  }
  return pairs;
}

bool getfullline(std::istream& in, std::string& line){
  std::string buffer;
  if(!std::getline(std::cin, buffer)){
    return false;
  }
  while(buffer.ends_with('\\')){
    buffer.pop_back();
    std::string addbuffer;
    if(!std::getline(std::cin, addbuffer)){
      throw "eof reached while combining lines";
    }
    buffer += addbuffer;
  }
  line = buffer;
  return true;
}


std::vector<mesh> loadobj(){
  std::string line;
  std::vector<glm::vec3> iverts;
  std::vector<glm::vec2> itexs;
  std::vector<glm::vec3> inorms;
  std::vector<mesh> model;
  while (getfullline(std::cin,line)){
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
      std::vector<indices> pairs = parse_f(iss);
      try{
        for(int i = 0; i < pairs.size()-2; i++){
          model.back().vertices.insert(end(model.back().vertices),{
            iverts.at(pairs[0].iv),iverts.at(pairs[i+1].iv),iverts.at(pairs[i+2].iv)
          });
          model.back().texturecoordinates.insert(end(model.back().texturecoordinates),{
            itexs.at(pairs[0].ivt),itexs.at(pairs[i+1].ivt),itexs.at(pairs[i+2].ivt)
          });
          model.back().vertexnormals.insert(end(model.back().vertexnormals),{
            inorms.at(pairs[0].ivn),inorms.at(pairs[i+1].ivn),inorms.at(pairs[i+2].ivn)
          });
        }
      }catch(std::out_of_range){
        throw "Invalid element index";
      }
    }
  }
  return model;
}

int main() {
  std::vector<mesh> model;
  try{
    model = loadobj();
  }catch(const char* s){
    std::cout << s << std::endl;
    return 1;
  }catch(std::string s){
    std::cout << s << std::endl;
    return 1; 
  }
  for(const auto & m : model){
    std::cout << m.meshName << std::endl;
    std::cout << m.texturecoordinates.size() << std::endl;
    std::cout << m.vertexnormals.size() << std::endl;
    std::cout << m.vertices.size() << std::endl;
  }
  return 0;
}
