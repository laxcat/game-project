// #pragma once
// #include <string>


// struct Path {
//     char full[1024];
//     char filenameOnly[256];
    
//     Path(std::string path = "") {
//         setPath(path.c_str());
//     }
    
//     void setPath(char const * p) {
//         // set full
//         snprintf(full, 1024, "%s", p);
//         // filenameOnly is everything after last slash
//         auto len = strlen(p);
//         while (len > 0 && p[len-1] != '/') --len;
//         snprintf(filenameOnly, 256, "%s", p+len);
//     }
// };
