#include <iostream>
#include <fstream>
#include <cmath>

#include <sys/stat.h>

#include "EasyBMP.h"

using namespace std;

bool fileExists(const string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}

float readFloat(istream& file){
    float x;
    file.read((char *) &x, sizeof(float));
    return x;
}

void writeFloat(ostream& file, float x){
    file.write((char *)&x, sizeof(float));
}

int readInt(istream& file)
{
    int val = 0;
    char bytes[4];

    file.read(bytes, 4);                                                          // read 4 bytes from the file
    val = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);       // construct the 16-bit value from those bytes (little endian)

    return val;
}

void writeInt(ostream& file, int val)
{
  char bytes[4];

  // extract the individual bytes from the value
  bytes[0] = (val);
  bytes[1] = (val >> 8);
  bytes[2] = (val >> 16);
  bytes[3] = (val >> 24);


  // write those bytes to the file
  file.write( (char*)bytes, 4 );
}

short readShort(istream& file)
{
    short val = 0;
    char bytes[2];

    file.read(bytes, 4);                // read 4 bytes from the file
    val = bytes[0] | (bytes[1] << 8);    // construct the 16-bit value from those bytes (little endian)

    return val;
}

void writeShort(ostream& file, short val)
{
  char bytes[2];

  // extract the individual bytes from our value
  bytes[0] = (val);
  bytes[1] = (val >> 8);

  // write those bytes to the file
  file.write( (char*)bytes, 2);
}


int convertImage(string inPath, string outPath){
    // variables
    BMP image;
    char bytes[4];
    int width = 0;
    int height = 0;
    int chunksize = 0;
    int i, j;

     // check if image exists
     if(!fileExists(inPath)){
        cerr << "Bilddatei nicht gefunden!" << endl;
        return -1;
     }

    // load the input image
    bool success = image.ReadFromFile(inPath.c_str());
    height = image.TellHeight();
    width = image.TellWidth();
    chunksize = height/16;

    // alert the User if the image could not be read
    if(!success){
        cerr << "Fehler: Bild konnte nicht gelesen werden!" << endl;
        cerr << "Moegliche Ursache: Bild ist mit Alphakanal gespeichert." << endl;
        cerr << "Bild als 24bpp (R8 G8 B8) speichern!" << endl;
        return -1;
    }

    cout << "Breite: " << width << " Hoehe: " << height << endl;

    // check if the dimensions match
    if(height != width){
        cerr << "Fehler: Bild ist nicht quadratisch" << endl;
        return -1;
    }
    // check if image is not too large
    else if(height > 512 || width > 512){
        cerr << "Fehler: Bild ist zu gross" << endl;
        return -1;
    }
    // check if dimensions are multiples of 16
    // this also means the image has to be at least 16x16
    else if(height%16 != 0 || width%16 != 0){
        cerr << "Fehler: Groesse des Bildes ist nicht durch 16 teilbar" << endl;
        return -1;
    }

    //create the output File
    ofstream outFile(outPath.c_str(), ifstream::binary);

    //write the start of the File
    outFile << "<Width>" << width << "</Width>\r\n" << "<Height>" << height << "</Height>\r\n";
    outFile << "<Clime>0</Clime>\r\n<Difficulty>0</Difficulty>\r\n";
    outFile << "<UsedChunks>\r\n\t<m_XSize>" << chunksize << "</m_XSize>\r\n\t<m_YSize>" << chunksize << "</m_YSize>\r\n";
    outFile << "\t<m_IntXSize>1</m_IntXSize>\r\n\t<m_BitGrid>CDATA[";

    // Fill the BitGrid with 1
    // this means every chunk is written in the file
    writeInt(outFile, 4*chunksize);
    bytes[0] = 0xFF; bytes[1] = 0xFF; bytes[2] = 0xFF, bytes[3] = 0xFF;
    for(i=0;i<chunksize;i++){
        outFile.write(bytes,4);
    }

    // fill in more hardcoded stuff TODO: CoastBuilding lines
    outFile << "]</m_BitGrid>\r\n</UsedChunks>\r\n<BuildBlockerShapes></BuildBlockerShapes>\r\n";
    outFile << "<DesertShapes></DesertShapes>\r\n<SurfLines></SurfLines>\r\n<CoastBuildingLines></CoastBuildingLines>\r\n";
    outFile << "<Lakes></Lakes>\r\n<Sandbanks></Sandbanks>\r\n<Fogbanks></Fogbanks>\r\n";
    outFile << "<TerrainNormalSplines></TerrainNormalSplines>\r\n<NativeSlots></NativeSlots>\r\n<Rivers></Rivers>\r\n<AIPoints></AIPoints>\r\n";
    outFile << "<ConstructionRecords></ConstructionRecords>\r\n<SendExplorationMessage>1</SendExplorationMessage>\r\n";
    outFile << "<m_GOPManager>\r\n\t<m_GRIDManager>\r\n\t\t<m_HeightMap>CDATA[";

    // heightmap header
    writeInt(outFile, width*height*2);

    // fill the heightmap
    // assumes greysacle image, so it only reads the red channel
    for(i=0;i<width*height;i++){
        RGBApixel temp = image.GetPixel(i%width,i/width);
        short val = (int)temp.Red*102.4-4096;
        writeShort(outFile, val);
    }

    outFile << "]\r\n\t\t</m_HeightMap>\r\n";

    // fill the Pathblocker layer with 0
    outFile << "\t\t<m_PathBlockerLayer>\r\n\t\t\t<BitGrid>\r\n\t\t\t\t<m_XSize>" << width*2 << "</m_XSize>\r\n\t\t\t\t";
    outFile << "<m_YSize>" << height*2 << "</m_YSize>\r\n\t\t\t\t<m_IntXSize>" << chunksize << "</m_IntXSize>\r\n\t\t\t\t<m_BitGrid>CDATA[";

    //header
    writeInt(outFile, (width*height)/2);
    bytes[0] = 0x00; bytes[1] = 0x00; bytes[2] = 0x00, bytes[3] = 0x00;

    // (size^2)/2 bytes filled with 00
    for(i=0;i<(width*height)/8;i++){
        outFile.write(bytes,4);
    }

    // fill in empty object lists and the header of the Terrain section
    outFile << "]\r\n\t\t\t\t</m_BitGrid>\r\n\t\t\t</BitGrid>\r\n\t\t</m_PathBlockerLayer>\r\n\t</m_GRIDManager>\r\n";
    outFile << "\t<Streets></Streets>\r\n\t<Objects>\r\n\t\t<Simple>\r\n\t\t\t<Objects></Objects>\r\n\t\t</Simple>\r\n\t\t<Handle>\r\n\t\t\t<Objects>";
    outFile << "</Objects>\r\n\t\t</Handle>\r\n\t\t<Nature>\r\n\t\t\t<Objects></Objects>\r\n\t\t</Nature>\r\n\t\t<Grass>\r\n\t\t\t<Objects></Objects>\r\n\t\t";
    outFile << "</Grass>\r\n\t</Objects>\r\n\t<SnapPartner></SnapPartner>\r\n\t<ObjectNames></ObjectNames>\r\n\t<ObjectGroups></ObjectGroups>\r\n";
    outFile << "</m_GOPManager>\r\n<Terrain>\r\n\t<Version>3</>\r\n\t<TileCountX>" << width <<"</>\r\n\t<TileCountZ>" << height <<"</>\r\n\t<TileCount>0</>\r\n\t";
    outFile << "<ChunkMap>\r\n\t\t<Width>" << chunksize << "</>\r\n\t\t<Height>" << chunksize << "</>";

    for(i=0;i<chunksize*chunksize;i++){
        outFile << "\r\n\t\t<Element>\r\n\t\t\t<VertexResolution>4</>\r\n\t\t\t<Flags>0</>\r\n\t\t\t<HeightMap>\r\n\t\t\t\t<Width>17</>\r\n\t\t\t\t<Data>CDATA[";

        // heightmap info
        // header
        bytes[0] = 0x84; bytes[1] = 0x04; bytes[2] = 0x00, bytes[3] = 0x00;
        outFile.write(bytes,4);

        // get offset
        int offX = (i%chunksize)*16;
        int offY = (i/chunksize)*16;

        // data
        // same as above, assumes greyscale so only uses red channel
        for(j=0;j<289;j++){
            RGBApixel temp = image.GetPixel(offX+(j%17),offY+(j/17));
            float val = (float)temp.Red/10-4.0;
            writeFloat(outFile, val);
        }

        outFile << "]</>\r\n\t\t\t</>\r\n\t\t\t<TexIndexData>\r\n\t\t\t\t<TextureIndex>0</>\r\n\t\t\t\t<AlphaMap>\r\n\t\t\t\t\t<Width>17</>\r\n\t\t\t\t\t<Data>CDATA[";

        // texture info
        // header
        bytes[0] = 0x21; bytes[1] = 0x01; bytes[2] = 0x00, bytes[3] = 0x00;
        outFile.write(bytes,4);

        // data
        // fill with 0xFF
        bytes[0] = 0xFF;
        for(j=0;j<289;j++){
            outFile.write(bytes,1);
        }

        outFile << "]</>\r\n\t\t\t\t</>\r\n\t\t\t</>\r\n\t\t</>";
    }

    outFile << "\r\n\t</>\r\n</>";

    outFile.close();
    return 0;
}

int main(int argc, char* argv[])
{
    SetEasyBMPwarningsOff(); // TODO: out of bounds warning

    // stores filenames
    string in = "input.bmp";
    string out = "output.isd";

    cout << "Konvertiert ein .bmp Bild in eine Anno-1404 Inseldatei." << endl;
    cout << "Im Moment wird nur die Heightmap bearbeitet, alles andere muss danach von Hand" << endl;
    cout << "erledigt werden." << endl << endl;
    cout << "Liest die Datei input.bmp ein und speichert in Datei output.isd, alternativ" << endl;
    cout << "koennen die Dateinamen als Aufrufparameter uebergeben werden." << endl << endl;
    cout << "Die Bitmap Datei muss ohne Alphakanal gespeichert werden, am besten eignen sich" << endl;
    cout << "24bit pro Pixel. Das Programm nimmt an, dass das Bild in Graustufen vorliegt." << endl << endl;
    cout << "Dieses Programm verwendet EasyBMP (http://easybmp.sourceforge.net/)." << endl << endl << endl;

    for(int i=1;i<argc;i++){          // iterate trough the arguments
        string argument = argv[i];    // store every second argument, starting with the first
        if(argument == "-help"){
            cout << "Moegliche Argumente:\n-help\n-input <dateiname>\n-output <dateiname>" << endl << endl;
            cout << "Beispiel: start IslandToBitmap.exe -input meinBild.bmp -output n_s_06.isd" << endl << endl;
            cout << "Zum beenden Fenster schliessen." << endl;
            cin.get();      // pause the program
            return 0;
        }
        else if(argument == "-input"){
            in = argv[i+1];             // store the next argument in the string
            i++; // increase i by one to skip the argument we just stored in in
        }
        else if(argument == "-output"){
            out = argv[i+1];             // store the next argument in the string
            i++; // increase i by one to skip the argument we just stored in out
        }
        else{
            cout << "Fehler: Argument " << argument << " wurde nicht erkannt!" << endl;
        }
    }

    // start conversion
    cout << "Lese " << in << " ein, Insel wird in " << out << " gespeichert." << endl;
    int result = convertImage(in, out);


    if(result == 0){
        cout << "Konvertierung erfolgreich. Zum beenden Fenster schliessen." << endl;
    }
    else{
        cout << "Fehler aufgetreten. Konvertierung wurde abgebrochen. Zum beenden Fenster schliessen." << endl;
    }

    cin.get();      // Pause the program

    return 0;
}
