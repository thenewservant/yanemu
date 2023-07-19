#include <cstdlib>
#include <cstdio>
#include <cstdint>

using namespace std;

class Rom{// basic for just NROM support
    private:
        uint8_t mapperType;
        uint8_t prgRomSize; // in 16kB units
        uint8_t chrRomSize; // in 8kB units
        uint8_t* prgRom;
        uint8_t* chrRom;
        
    public: 
    //constructor is 
        Rom(FILE *romFile){
            if (romFile == NULL){
                printf("Error: ROM file not found");
                exit(1);
            }
            uint8_t header[16];
        
            fread(header, 1, 16, romFile);
            if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A){
                printf("Error: ROM file is not a valid NES ROM");
                exit(1);
            }
            mapperType = ((header[6] & 0xF0)>> 4) | (header[7] & 0xF0);
            prgRomSize = header[4];
            chrRomSize = header[5];
            prgRom = (uint8_t*)malloc(prgRomSize * 0x4000);
            chrRom = (uint8_t*)malloc(chrRomSize * 0x2000);
            fread(prgRom, 1, prgRomSize * 0x4000, romFile);
            fread(chrRom, 1, chrRomSize * 0x2000, romFile);
        }

        void printInfo(){
            printf("Mapper type: %d \n", mapperType);
            printf("PRG ROM size: %d \n", prgRomSize);
        }
        
        uint8_t readPrgRom(uint16_t addr){
            return prgRom[addr];
        }

        uint8_t readChrRom(uint16_t addr){
            return chrRom[addr];
        }
        uint8_t getChrRomSize(){
            return chrRomSize;
        }
        ~Rom(){
            free(prgRom);
            free(chrRom);
        }
};

int main(){
    FILE *romFile = fopen("C:\\Users\\ppd49\\3D Objects\\C++\\yanemu\\smbSale.nes", "rb");
    Rom rom(romFile);
    rom.printInfo();

    for (int i = 0; i < rom.getChrRomSize()*0x2000; ++i){
        if (i % 8 == 0)
            printf("\n");
        if (i % 64 == 0)
            printf("\n");
        printf("%X ", rom.readChrRom(i));
    }
    
}