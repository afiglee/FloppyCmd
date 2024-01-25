//
//  FAT.hpp
//  FloppyCmd
//
//  Created by Dmitriy Fitisov on 2/1/22.
//

#ifndef FAT_hpp
#define FAT_hpp

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <ctime>
#include <vector>

const unsigned int ATTR_READ_ONLY = 0x01;
const unsigned int ATTR_HIDDEN    = 0x02;
const unsigned int ATTR_SYSTEM    = 0x04;
const unsigned int ATTR_VOLUME_ID = 0x08;
const unsigned int ATTR_DIRECTORY = 0x010;
const unsigned int ATTR_ARCHIVE   = 0x020;


typedef struct  __attribute__((packed)) sMBRHeader{
    uint8_t  BS_jmp[3];    // 0xEB0090
    char     BS_OEMname[8]; //any name
    uint16_t BPB_BytsPerSec; //512, 1024, 2048 or 4096
    uint8_t  BPB_SecPerClus; //1,2,4,8,16,32,64,128
    uint16_t BPB_RsvdSecCnt;
    uint8_t  BPB_NumFATs; //1 or 2
    uint16_t BPB_RootEntCnt; //multiple of BPB_BytsPerSec/32
    uint16_t BPB_TotSec16;
    uint8_t  BPB_Media; //0xF0 (removable) 0xF8..0xFF(fixed)
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint8_t  BS_DrvNum;
    uint8_t  BS_Reserved1;
    uint8_t  BS_BootSig;
    uint32_t BS_VollD;
    char     BS_VolLab[11];
    char     BS_FilSysType[8];
    uint8_t  data[448];
    uint16_t Signature;
}MBRHeader;

std::ostream & operator<<(std::ostream & os, const MBRHeader &hdr);

typedef struct __attribute__((packed)) DirEntry{
    char     DirName[11];
    uint8_t  DirAttr;
    uint8_t  DirNTRes;
    uint8_t  DirCrtTimeTenth;
    uint16_t DirCrtTime;
    uint16_t DirCrtDate;
    uint16_t DirLstAccDate;
    uint16_t DirFstCkusHI;
    uint16_t DirWrtTime;
    uint16_t DirWrtDate;
    uint16_t DirFstClusLO;
    uint32_t DirFileSize;
} DirEntry;



class FATEntry {
public:
    //FATEntry();
    FATEntry(const DirEntry& entry);
    
    const std::string getEntryName() const;
    const std::string getAttrStr() const;
    size_t getSize() const;
    const std::string getWriteDateTime() const;
    const std::string getCreateDateTime() const;
    
    friend std::ostream & operator<<(std::ostream &os, const FATEntry& dir);
protected:
    static std::string convertDate(uint16_t date);
    static std::string convertTime(uint16_t time);
    
    DirEntry dEntry;

};

class FAT {
public:
    FAT();
    FAT(FILE *f);
    FAT(const MBRHeader &header);
    
    virtual ~FAT();
    
    const MBRHeader & getHeader() const;
    
    friend std::ostream & operator<<(std::ostream &os, const FAT& fat);
    void readDir(std::vector<FATEntry> &dirEntries);
protected:
    void init(MBRHeader &header);
    
    void calcStarts() ;
    
    FILE *f;
    MBRHeader header;
    size_t fatStart;
    size_t rootStart;
    size_t clusters;
    
};



#endif /* FAT_hpp */
