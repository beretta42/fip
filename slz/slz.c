/*****************************************************************************
     SLZ - SLZ (un)packer.            C. 1993 SilverFox SoftWare.
        Written by Adisak Pochanayon     All Rights Reserved.

          This program is a simple example of using SLZ compression
     upon a file.  It will allow you to both pack and unpack a file
     using SLZ compression.  The syntax for calling this program from
     the CLI is:

               SLZ  FLAG  SOURCE  DESTINATION
               FLAG can be U - unpack  P - pack

*****************************************************************************/

// Comment out the following line to save 256K of memory but can slow down
//   compression (immensely :( ).
//#define hashing_on

#include  <stdio.h>
#include  <stdlib.h>

/*  defines for interpreting the CLI arguments    */
#define   arg_flag     1
#define   arg_srce     2
#define   arg_dest     3
#define   arg_count    4

/*  defines for handling argument and file errors */
#define   slz_error   1024
#define   error_read   0
#define   error_args   1
#define   error_srce   2
#define   error_dest   4
#define   error_write  8

/*  defines for file compression / decompression  */
#define   FILE_ERROR   0
#define   TRUE         1
#define   FALSE        0

/*** GLOBAL VARIABLE FOR ERRORS AND FILES ***/
int  error=0;
long length;
short lzhist_offset=0;

/*
 * SHIFT_UPPER is amount to multiply upper in one byte to get into next
 *   higher byte. (H=4096 -> 16, H=2048 -> 8)
 * LSR_upper is amount to shift codeword to get upper byte into lower byte.
 *   (H=4096 -> 4, H=2048 -> 3)
 * MAX_COMP_LENGTH = (2 ^ (8-LSR_upper)) + 1
 */

#define HISTORY_SIZE     4096
#define MASK_history     (HISTORY_SIZE-1)
#define MASK_upper       (0xF0)
#define MASK_lower       (0x0F)
#define SHIFT_UPPER      16
#define LSR_upper        4
#define MAX_COMP_LEN     17


unsigned char LZ_history[HISTORY_SIZE];
#ifdef hashing_on
long HASH_TABLE[65536];
#define CHASH(a,b,c)  ( ((((a&63)<<6)+(b&63))<<4)+c )
#endif

void writechar(register unsigned char outchar, register FILE *outfile)
{
  if (putc(outchar,outfile)==EOF)
    {
      printf("Error writing output file.\n");
      /* fcloseall(); */
      exit(slz_error + error_write);
    }
  LZ_history[lzhist_offset]=outchar; lzhist_offset=(lzhist_offset+1)&MASK_history;
}

void UnPackSLZ(unsigned char *inbuffer, register FILE *outfile)
{
  short myTAG, mycount, myoffset;
  long int loop1;

  int dist=0;
  int min=0;

  for(;;)  // loop forever (until goto occurs to break out of loop)
    {
      myTAG=*inbuffer++;
      dist++;
      for(loop1=0;(loop1!=8);loop1++)
        {
          if(myTAG&0x80)
            {
              if((mycount=*inbuffer++)==0)  // Check EXIT
                { goto skip2; } // goto's are gross but it's efficient :(
              else
                {
                  dist++;
                  myoffset=HISTORY_SIZE-(((MASK_upper&mycount)*SHIFT_UPPER)+(*inbuffer++));
                  mycount&=MASK_lower;
                  mycount+=2;
                  while(mycount!=0)
                    {
                      writechar(LZ_history[(lzhist_offset+myoffset)&MASK_history],outfile);
		      dist--;
		      if( dist < min ) min = dist;
                      mycount--;
                    }
                }
            }
          else
            { writechar(*inbuffer++,outfile); }
          myTAG+=myTAG;
        }
    }
skip2:
  printf("min dist: %d\n", min );
  return;
}

#ifdef hashing_on
void ADDHASH(long a,long b, long c)
{
  long *INHASH,*BEFINHASH;
  INHASH=&HASH_TABLE[CHASH(a,b,15)];
  BEFINHASH=INHASH-1;
  *(INHASH--)=*(BEFINHASH--); *(INHASH--)=*(BEFINHASH--); *(INHASH--)=*(BEFINHASH--);
  *(INHASH--)=*(BEFINHASH--); *(INHASH--)=*(BEFINHASH--); *(INHASH--)=*(BEFINHASH--);
  *(INHASH--)=*(BEFINHASH--); *(INHASH--)=*(BEFINHASH--); *(INHASH--)=*(BEFINHASH--);
  *(INHASH--)=*(BEFINHASH--); *(INHASH--)=*(BEFINHASH--); *(INHASH--)=*(BEFINHASH--);
  *(INHASH--)=*(BEFINHASH--); *(INHASH--)=*(BEFINHASH--); *(INHASH)=*(BEFINHASH);
  *BEFINHASH=c;  
}
#endif

void PackSLZ(unsigned char *inbuffer, register FILE *outfile)
{
  long int loop1,loop2, startvalue, endvalue, wherefound;
  unsigned char outchars[MAX_COMP_LEN*10];
  unsigned char *a_buffer, *b_buffer;
  short lenoutchars;
  short myTAG, iter;
  short done,found,temp;

#ifdef hashing_on
  long *INHASH;
  short HASHCHECK;

  for(loop1=0;loop1!=65536;loop1++)
    {
      HASH_TABLE[loop1]=-1;
    }
  HASH_TABLE[CHASH(inbuffer[0],inbuffer[1],0)]=0;
#endif

  loop1=1; loop2=length-1; iter=1;  // Always do a literal first byte
  outchars[1]=inbuffer[0]; lenoutchars=2;
  done=FALSE;

  while(! done)
    {
      myTAG=0;
      while((iter<8)&&(! done))
        {

/***--- This is the slowest part.  I should rewrite it in ASM ---***/

          startvalue=loop1-MASK_history;
          if (startvalue<0) startvalue=0;
          found=0;

#ifdef hashing_on
          INHASH=&HASH_TABLE[CHASH(inbuffer[loop1],inbuffer[loop1+1],0)];
          HASHCHECK=16; // Scan for matches in HASH TABLE
          while((HASHCHECK)&&((endvalue=*INHASH++)>=startvalue))
            {
              temp=0;
              a_buffer=&inbuffer[loop1];
              b_buffer=&inbuffer[endvalue];
              while((*(a_buffer++) == *(b_buffer++))&&(temp<MAX_COMP_LEN))
                    {
		      temp++;
		    }
	      if(found<temp)
		{
		  if ((temp+loop1)>length)
		    {
                      temp=length-loop1;
                      if(found<temp) 
			{
			  found=temp;
			  wherefound=endvalue;
			}
		    }
		  else
		    {
		      found=temp;
		      wherefound=endvalue;
		    }
		  if (found=MAX_COMP_LEN) { goto skip1; }
		}
	      HASHCHECK--;
	    }
#else
	  endvalue=loop1-1;
#endif

	  while( endvalue>=startvalue) // Scan for matches
            {
              temp=0;
              a_buffer=&inbuffer[loop1];
              b_buffer=&inbuffer[endvalue];
              while((*(a_buffer++) == *(b_buffer++))&&(temp<MAX_COMP_LEN))
		{
		  temp++;
		}
	      if( found<temp)
		{
		  if ((temp+loop1)>length)
		    {
		      temp=length-loop1;
		      if( found<temp)
			{
			  found=temp;
			  wherefound=endvalue;
			}
		    }
		  else
		    {
		      found=temp;
		      wherefound=endvalue;
		    }
		  if ( found==MAX_COMP_LEN) { goto skip1; }
		}
	      endvalue--;
	    }

	skip1:
	  
	  if(found>2) // Compression will now occur
            {
              myTAG|=(128>>iter);
              endvalue=loop1-wherefound;
              startvalue=((endvalue>>LSR_upper)&MASK_upper)+(found-2);
              outchars[lenoutchars++]=startvalue;
              outchars[lenoutchars++]=(unsigned char)endvalue;
              loop2-=found;
              while(found!=0)
                {
#ifdef hashing_on
                 ADDHASH(inbuffer[loop1],inbuffer[loop1+1],loop1);
#endif
                  loop1++; found--;
                }
            }
          else // No Compression, copy literal byte
            {
#ifdef hashing_on
              ADDHASH(inbuffer[loop1],inbuffer[loop1+1],loop1);
#endif
              outchars[lenoutchars++]=inbuffer[loop1++];
              loop2--;
            }
          if(loop2<=0)
            {
              done=TRUE;
              if(loop2<0)
                {
                  printf("HMMM... ooops!!!\n");
                }
            }
          iter++;
        }
      if(! done)
        {
          outchars[0]=myTAG;
          for(startvalue=0;startvalue!=lenoutchars;startvalue++)          
            {
//            writechar(outchars[startvalue],outfile);
/*****/
  if (putc(outchars[startvalue],outfile)==EOF)
    {
      printf("Error writing output file.\n");
      /* fcloseall(); */
      exit(slz_error + error_write);
    }
/*****/

            }
          lenoutchars=1;
          iter=0;
        }
    }
  if(iter==8)
    {
      outchars[0]=myTAG;
      for(startvalue=0;startvalue!=lenoutchars;startvalue++)          
        {
          writechar(outchars[startvalue],outfile);
        }
      writechar(255,outfile); writechar(0,outfile); writechar(0,outfile);
    }
  else
    {
      outchars[0]=myTAG|(128>>iter);
      for(startvalue=0;startvalue!=lenoutchars;startvalue++)          
        {
          writechar(outchars[startvalue],outfile);
        }
      writechar(0,outfile);
      //
      // Brett Gordon Modified to spit out 2 zero for
      // the end of image marker, as it should be a word long
      // per SLZ specks.
      // 6/8/2013
      //
      writechar(0,outfile);
    }
}

main(int argc, char *argv[])
{
  FILE *infile, *outfile;
  unsigned char *inbuffer;

  printf(" SLZ -- SLZ (un)packer.          C. 1993 SilverFox SoftWare.\n");
  printf("   Written by Adisak Pochanayon     All Rights Reserved.\n\n");

  if (argc != arg_count)
    {
      error= slz_error + error_args;
      printf("Error -- Please check arguments.\n");
    }
  else if ((infile  = fopen(argv[arg_srce],"r"))==FILE_ERROR) 
    {
      error= slz_error + error_srce;
      printf("Error -- SOURCE file could not be opened.\n");
    }
  else if ((outfile = fopen(argv[arg_dest],"w"))==FILE_ERROR)
    {
      error= slz_error + error_dest;
      printf("Error -- DESTINATION file could not be opened.\n");
    }
  else
    {
/*
 * Get the length of the input file (sorry it takes me 3 calls :( )
 * If you have a better way, let me know.
 */
      fseek(infile,0,2); length=ftell(infile); rewind(infile);

      if (length>2) if ((inbuffer=(unsigned char *)calloc(length,1))!=0)
        {
          fread(inbuffer,1,length,infile);  // assume this goes no problem :)
          switch (*argv[arg_flag])
            {
              case 'U' : case 'u' : printf("UnPacking File %s.\n",argv[arg_srce]);
                         UnPackSLZ(inbuffer, outfile); break;
              case 'P' : case 'p' : printf("Packing File %s.\n",argv[arg_srce]); 
                         PackSLZ(inbuffer, outfile);   break;
              default  : error=slz_error + error_args; printf("Error -- no FLAG selected.\n");
            }
          free(inbuffer);
        }
    }

  if (error!=0)
     printf("\nSLZ command syntax is:  \"SLZ  FLAG  SOURCE  DESTINATION\"\n   where FLAG can be U - unpack  P - pack.\n\n");
  else
     printf("\nSLZ successfully completed\n");

  /* fcloseall(); */
  exit(error);
}
