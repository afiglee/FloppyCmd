//
//  FAT.cpp
//  FloppyCmd
//
//  Created by Dmitriy Fitisov on 2/1/22.
//

#include "FAT.hpp"
#include <iomanip>
#include <ios>
#include <sstream>


FATEntry::FATEntry(const DirEntry& entry){
    this->dEntry = entry;
}

const std::string FATEntry::getEntryName() const {
    std::string name;
    for (size_t tt = 0; tt < 8; tt++) {
        if (dEntry.DirName[tt] != ' ' && dEntry.DirName[tt] != 0) {
            name += dEntry.DirName[tt];
        } else {
            break;
        }
    }
    if (!name.empty() && dEntry.DirName[8] != ' ' && dEntry.DirName[8] != 0) {
        name += '.';
    }
    for (size_t tt = 8; tt < 11; tt++) {
        if (dEntry.DirName[tt] == ' ' || dEntry.DirName[tt] == 0) {
            break;
        }
        name += dEntry.DirName[tt];
    }
    return name;
}

const std::string FATEntry::getAttrStr() const {
    std::string attr;
    if (dEntry.DirAttr & ATTR_DIRECTORY){
        attr += "D";
    }
    if (dEntry.DirAttr & ATTR_SYSTEM){
        attr += "S";
    }
    if (dEntry.DirAttr & ATTR_READ_ONLY){
        attr += "R";
    }
    if (dEntry.DirAttr & ATTR_ARCHIVE){
        attr += "A";
    }
    if (dEntry.DirAttr & ATTR_HIDDEN){
        attr += "H";
    }
    if (dEntry.DirAttr & ATTR_VOLUME_ID){
        attr += "H";
    }
    return attr;
}

size_t FATEntry::getSize() const {
    return dEntry.DirFileSize;
}

const std::string FATEntry::getWriteDateTime() const {
    return FATEntry::convertTime(dEntry.DirWrtTime) +
    " " + FATEntry::convertDate(dEntry.DirWrtDate);
}

std::string FATEntry::convertDate(uint16_t date){
    std::stringstream sDate;
    sDate << (date & 0x1F);
    sDate << "/";
    sDate << ((date & 0x1E0) >> 5);
    sDate << "/";
    sDate << (1980 + ((date & 0xFE00) >> 9));
    return sDate.str();
}

static void prepend0(std::stringstream &ss, int val){
    if (!val) {
        ss << "00";
    } else  {
        if (val < 10) {
            ss << "0";
        }
        ss << val;
    }
}
std::string FATEntry::convertTime(uint16_t time){
    std::stringstream sTime;
    int val = ((time & 0xF800) >> 11);
    prepend0(sTime, val);
    sTime << ":";
    val = ((time & 0x7E0) >> 5);
    prepend0(sTime, val);
    sTime << ":";
    val = ((time & 0x1F));
    prepend0(sTime, val);
    return sTime.str();
}


const std::string FATEntry::getCreateDateTime() const {
    std::string dateTime;
    
    return dateTime;
}




FAT::FAT() {
    f = NULL;
    init(header);
    calcStarts();
}

FAT::FAT(FILE *f) {
    if (f){
        if (fseek(f, 0, SEEK_SET)) {
            throw std::runtime_error("Failed to move to root directory");
        }
        size_t count = fread(&header, sizeof(header), 1, f);
        if (count < 1) {
            throw std::length_error("Truncated Master Boot Record");
        }
        calcStarts();
    }
    this->f = f;
}

FAT::FAT(const MBRHeader &header) {
    f = NULL;
    this->header = header;
    calcStarts();
}

FAT::~FAT() {
    if (f) {
        fclose(f);
    }
}

const MBRHeader & FAT::getHeader() const {
    return header;
}


void FAT::init(MBRHeader &header) {
    header.BS_jmp[0] = 0xEB;
    header.BS_jmp[1] = 0x3C;
    header.BS_jmp[2] = 0x90;
    strncpy(header.BS_OEMname, "FAT12   ", 8);
    header.BPB_BytsPerSec = 512; //floppy
    header.BPB_SecPerClus = 1;
    header.BPB_RsvdSecCnt = 1;
    header.BPB_NumFATs = 2;
    header.BPB_RootEntCnt = 14;
    header.BPB_TotSec16 = 0x0B40; //2880 - 1.44 floppy
    header.BPB_Media = 0xF0;
    header.BPB_FATSz16 = 0x09;
    header.BPB_SecPerTrk = 0x012;
    header.BPB_NumHeads = 0x02;
    header.BPB_HiddSec = 0;
    header.BPB_TotSec32 = 0;
    header.BS_DrvNum = 0;
    header.BS_Reserved1 = 0;
    header.BS_BootSig = 0x29;
    header.BS_VollD = static_cast<unsigned int>(std::time(nullptr));
    strncpy(header.BS_VolLab, "NO NAME    ", 11);
    strncpy(header.BS_FilSysType, "FAT12   ", 8);
    header.Signature = 0xAA55;
    calcStarts();
}

void FAT::calcStarts() {
    clusters = (((header.BPB_TotSec16 ? header.BPB_TotSec16 : header.BPB_TotSec32) -
            header.BPB_RsvdSecCnt - header.BPB_NumFATs * header.BPB_FATSz16 -
            (header.BPB_RootEntCnt * sizeof(DirEntry) / header.BPB_BytsPerSec))/header.BPB_SecPerClus);
    fatStart = header.BPB_RsvdSecCnt * header.BPB_BytsPerSec;
    rootStart = fatStart + header.BPB_FATSz16 * header.BPB_NumFATs * header.BPB_BytsPerSec;
}

void FAT::readDir(std::vector<FATEntry> &dirEntries){
    if (!f) {
        throw std::logic_error("No open stream");
    }
    DirEntry entry;
    if (fseek(f, rootStart, SEEK_SET)) {
        throw std::runtime_error("Failed to move to root directory");
    }
    for (size_t tt = 0; tt < header.BPB_RootEntCnt; tt++) {
        if (fread(&entry, sizeof(DirEntry), 1, f) != 1) {
            throw std::runtime_error("Failed to read dir entry");
        }
        if (!entry.DirName[0]) {
            break;
        }
        if ((uint8_t) entry.DirName[0] != 0xE5) {
            dirEntries.push_back(FATEntry(entry));
        }
    }
}

std::ostream & operator<<(std::ostream & os, const MBRHeader &hdr){
    std::cout << "BS_OEMname: "     << hdr.BS_OEMname       << std::endl;
    std::cout << "BPB_BytsPerSec: " << hdr.BPB_BytsPerSec   << std::endl;
    std::cout << "BPB_SecPerClus: " << (int) hdr.BPB_SecPerClus   << std::endl;
    std::cout << "BPB_RsvdSecCnt: " << hdr.BPB_RsvdSecCnt   << std::endl;
    std::cout << "BPB_NumFATs: "    << (int) hdr.BPB_NumFATs      << std::endl;
    std::cout << "BPB_RootEntCnt: " << hdr.BPB_RootEntCnt   << std::endl;
    std::cout << "BPB_TotSec16: "   << hdr.BPB_TotSec16     << std::endl;
    std::cout << "BPB_Media: "      << std::showbase  << std::setbase(16) << (int) hdr.BPB_Media        << std::endl;
    std::cout << "BPB_FATSz16: "    << std::setbase(10) << hdr.BPB_FATSz16      << std::endl;
    std::cout << "BPB_SecPerTrk: "  << hdr.BPB_SecPerTrk    << std::endl;
    std::cout << "BPB_NumHeads: "   << hdr.BPB_NumHeads     << std::endl;
    std::cout << "BPB_HiddSec: "    << hdr.BPB_HiddSec      << std::endl;
    std::cout << "BPB_TotSec32: "   << hdr.BPB_TotSec32     << std::endl;
    std::cout << "BS_DrvNum: "      << (int) hdr.BS_DrvNum        << std::endl;
    std::cout << "BS_Reserved1: "   << (int) hdr.BS_Reserved1     << std::endl;
    std::cout << "BS_BootSig: "     << std::showbase  << std::setbase(16) << (int) hdr.BS_BootSig       << std::endl;
    std::cout << "BS_VollD: "       << std::showbase  << std::setbase(16) << hdr.BS_VollD         << std::endl;
    std::cout << "BS_VolLab: '"      << std::string(hdr.BS_VolLab, sizeof(hdr.BS_VolLab))        << "'" <<std::endl;
    std::cout << "BS_FilSysType: '"  << std::string(hdr.BS_FilSysType, sizeof(hdr.BS_FilSysType))  << "'"  << std::endl;
    std::cout << "Signature: " << std::showbase  << std::setbase(16) << hdr.Signature        << std::endl;

    return os;
}

std::ostream & operator<<(std::ostream &os, const FAT& fat) {
    os << fat.header;
    os << std::setbase(10) << "fatStart=" << fat.fatStart << std::endl;
    os << "rootStart=" << fat.rootStart << std::endl;
    os << "clusters=" << fat.clusters << std::endl;
    return os;
}

std::ostream & operator<<(std::ostream &os, const FATEntry& fEntry) {
    size_t prep = 16;
    std::string name = fEntry.getEntryName();
    
    prep -= name.size();
    //std::cout << "size=" << name.size() <<  " prep=" << prep << std::endl;
    /*os << "DirName= " << fEntry.getEntryName() << std::endl;
    os << "DirAttr= " << fEntry.getAttrStr() << std::endl;
    //os << "DirNTRes= " << (int) dir.DirNTRes << std::endl;
    //os << "DirCrtTimeTenth= " << (int) dir.DirCrtTimeTenth << std::endl;
    //os << "DirCrtTime= " << (int) dir.DirCrtTime << std::endl;
    //os << "DirCrtDate= " << (int) dir.DirCrtDate << std::endl;
    //os << "DirLstAccDate= " << (int) dir.DirLstAccDate << std::endl;*/
    os << "DirFstCkusHI= " << (int) fEntry.dEntry.DirFstCkusHI << std::endl;
    /*os << "DirWrtTime= " << std::showbase  << std::setbase(16) << fEntry.dEntry.DirWrtTime << std::endl;
    os << "DirWrtDate= " <<  fEntry.dEntry.DirWrtDate << std::endl;*/
    os << "DirFstClusLO= " << (int) fEntry.dEntry.DirFstClusLO << std::endl;
    /*os << "WriteDate=" << fEntry.getWriteDateTime() << std::endl;
    os << "DirFileSize= " << std::setbase(10) << fEntry.getSize() << std::endl;*/
    os << name //fEntry.getEntryName()
       << std::string(prep, ' ')
       << std::setw(10)
       << fEntry.getSize()
       << "  "
       << fEntry.getWriteDateTime()
       << "  "
    << fEntry.getAttrStr();
    //<< std::endl;
    return os;
}
