//
//  main.cpp
//  FloppyCmd
//
//  Created by Dmitriy Fitisov on 2/1/22.
//

#include <iostream>
#include "FAT.hpp"
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    char buf[MAXPATHLEN];
    std::cout << getcwd(buf, sizeof(buf)) << std::endl;
   
    
    if (argc != 2) {
        std::cout << "Filename of the floppy image required" << std::endl;
        return 1;
    }
    std::cout << "File to open: " << argv[1] << std::endl;
    
    FILE *f = fopen(argv[1], "r+");
    if (!f) {
        std::cout << "Error " << errno << " to open file " << argv[1] << std::endl;
        return 2;
    }
    try {
        FAT fat(f);
        std::cout << fat;
        std::vector<FATEntry> dirs;
    
        fat.readDir(dirs);
        std::cout << "dirs: " << dirs.size() << std::endl;
        for (auto dir:dirs){
            std::cout << dir;
            std::cout << std::endl;
        }
    } catch (std::exception &ex) {
        std::cout << ex.what();
    }
    /*
    MBRHeader header;
    size_t count = fread(&header, 1, sizeof(header), f);
    if (count < 512) {
        std::cout << "Read " << count << " bytes from 512 " << ferror(f) << std::endl;
    } else {
        FAT fat(header);
        std::cout << header;
    }*/
    
    //fclose(f);
    
    return 0;
}
