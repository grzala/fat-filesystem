#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "filesys.h"


void write_text_to_file(char *to_save, MyFILE* file)
{
   for (int i = 0; i < strlen(to_save); i++)
   {
      char a = to_save[i];
      myfputc(a, file);
   }
}

void build_string_from_file(char *string, MyFILE* file)
{
   int index = 0;
   while (index <= file->filelength) 
   {
      string[index] = myfgetc(file);
      index++;
   }
}

int main()
{
   //load disk
   //I am not formatting the disk because there is no need to
   readdisk("virtualdisk");
   
   format();
   
   //need this for printing direcotry list
   char** list;
   
   mymkdir("/firstdir"); //I deliberately do not support recursive mkdir
   mymkdir("/firstdir/seconddir");
   
   
   MyFILE* file = myfopen("/firstdir/seconddir/testfile1.txt", "w");
   const char* string = "Bacon ipsum dolor amet filet mignon pork chop turducken ribeye, beef ribs spare ribs shank corned beef t-bone meatloaf cupim tail shoulder. Turkey bresaola biltong, shank ham hock ball tip meatloaf pancetta flank brisket cupim short loin beef strip steak. Brisket ball tip tongue, jowl flank beef andouille short loin porchetta bresaola pork belly swine capicola. Strip steak meatloaf spare ribs leberkas bresaola porchetta landjaeger. Biltong pork chop ribeye, tongue turducken andouille short ribs porchetta ham hock jerky landjaeger frankfurter. Boudin landjaeger kevin porchetta beef ribs corned beef tail.";
   //put some text in file
   for (int i = 0; i < 1.5*BLOCKSIZE; i++) {
      myfputc(string[i % strlen(string)], file);   
   }
   myfclose(file);
   
   list = mylistdir("/firstdir/seconddir");
   printf("Contents of /firstdir/seconddir:\n");
   printdirs(list);
   
   mychdir("/firstdir/seconddir");
   
   list = mylistdir(".");
   printf("Contents of . :\n");
   printdirs(list);
   
   MyFILE* file2 = myfopen("testfile2.txt", "w");
   write_text_to_file("example test", file2);
   myfclose(file2);
   
   mymkdir("thirddir");
   MyFILE* file3 = myfopen("thirddir/testfile3.txt", "w");
   write_text_to_file("more example text", file3);
   myfclose(file3);
   
   mymkdir("/copydir");
   myfcp("thirddir/testfile3.txt", "/copydir/testfile3.txt");
   
   writedisk("virtualdisk2");   
   
   myremove("testfile1.txt");
   myremove("testfile2.txt");
   
   mychdir("thirddir");
   myremove("testfile3.txt");
   
   mychdir("..");
   
   myrmdir("thirddir");
   mychdir("/firstdir");
   myrmdir("seconddir");
   mychdir("/");
   myrmdir("firstdir");
   
   myremove("/copydir/testfile3.txt");
   myrmdir("/copydir");
   
   writedisk("virtualdisk");
   
   return 0;
}
