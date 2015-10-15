// Inline editor for TX81z (MIDI-CC to TX81z SYX)
// inline_TX81z.c
//
// 20151015
// GNU GENERAL PUBLIC LICENSE Version 2
// Daniel Skaborn

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
	FILE *infil;
	FILE *utfil;

	int fd;
	int wd;

	unsigned char mapped[256][8];
	unsigned char mask[256][8];
	unsigned char page, rd;
	unsigned char noofpages=4;
	unsigned char parametervalue[256];
	unsigned char parameterstatus[256];
	unsigned char syxoutblock[11];

	int a,b,c,d,e,f,g,h, cntrl;
	unsigned char device[255];
	unsigned char tmp[255];
	unsigned char cs=0;
	int syxfilecounter = 0;

	int i, ii, mps, notallset;
	unsigned char channel = 0;
	int infilstate = 0;

	unsigned char inbuf;
	unsigned char utbuf;
	unsigned char controller;
	size_t nbytes;
	ssize_t x;

	printf("\n\nInline MIDI ControlChange to Yamaha TX81z syx parameter set\nWritten by Daniel Skaborn 2015\n\n");

	infil=fopen("mapping.tx81zmp","r");

  // reset all mapping
	
	for (i=0;i<256;i++) {
		for (ii=0;ii<8;ii++) {
			mapped[i][ii]=0xfd;
			mask[i][ii]=0x7F;
		}
		parameterstatus[i]=0;
	}
	
	// parameters not used...
	for (i=87;i<93;i++)
		parameterstatus[i]=2;
	for (i=94;i<128;i++)
		parameterstatus[i]=2;
	for (i=0x97;i<256;i++)
		parameterstatus[i]=2;
				
	if (infil==NULL) {
		printf("Could not open mapping file\n");
		return -1;
	}

	infilstate = 0;
	i=0;
	while(!feof(infil)) {
		rd=fgetc(infil);
		switch(infilstate) {
			case 0: // read device
				if ((rd=='\n') || (rd=='\r')) {
					device[i] = '\0';
					infilstate=1; i=0;
					break;
				}
				device[i]=rd; i++;
				break;
			case 1: // read midi channel
				if ((rd=='\n') || (rd=='\r')) {
					tmp[i]='\0'; i=0;
					sscanf(tmp,"%d",&channel);
					channel--;
					if (channel<0) channel = 0;
					if (channel>15) channel = 15;
					printf("MIDI channel = %d\n", channel+1);
					infilstate=2;
					break;
				}
				tmp[i]=rd; i++;
				break;
			case 2: // read number of pages
				if ((rd=='\n') || (rd=='\r')) {
					tmp[i]='\0'; i=0;
					sscanf(tmp,"%d",&noofpages);
					noofpages--;
					if (noofpages<1) noofpages = 1;
					if (noofpages>8) noofpages = 8;
					printf("pages = %d\n", noofpages+1);
					infilstate=3;
					break;
				}
				tmp[i]=rd; i++;
				break;
			case 3: // read mapping
				if ((rd=='\n') || (rd=='\r')) {
					tmp[i]='\0'; i=0;
					a=0; b=0; c=0;
					sscanf(tmp,"%x %x %x %x %x %x %x %x %x",&cntrl, &a,&b,&c,&d,&e,&f,&g,&h);
					mapped[(unsigned char)(cntrl)][0]=(unsigned char)(a);
 					mapped[(unsigned char)(cntrl)][1]=(unsigned char)(b);
					mapped[(unsigned char)(cntrl)][2]=(unsigned char)(c);
					mapped[(unsigned char)(cntrl)][3]=(unsigned char)(d);
 					mapped[(unsigned char)(cntrl)][4]=(unsigned char)(e);
 					mapped[(unsigned char)(cntrl)][5]=(unsigned char)(f);
					mapped[(unsigned char)(cntrl)][6]=(unsigned char)(g);
					mapped[(unsigned char)(cntrl)][7]=(unsigned char)(h);
					infilstate=4;
					if (a==0xFF) { // no more to read.
						infilstate=5;
					}
					break;
				}
				tmp[i]=rd; i++;
				break;
			case 4: // read masking
				if ((rd=='\n') || (rd=='\r')) {
					tmp[i]='\0'; i=0;
					a=0; b=0; c=0;
					sscanf(tmp,"%x %x %x %x %x %x %x %x %x",&cntrl, &a,&b,&c,&d,&e,&f,&g,&h);
					mask[(unsigned char)(cntrl)][0]=(unsigned char)(a);
					mask[(unsigned char)(cntrl)][1]=(unsigned char)(b);
					mask[(unsigned char)(cntrl)][2]=(unsigned char)(c);
					mask[(unsigned char)(cntrl)][3]=(unsigned char)(d);
					mask[(unsigned char)(cntrl)][4]=(unsigned char)(e);
					mask[(unsigned char)(cntrl)][5]=(unsigned char)(f);
					mask[(unsigned char)(cntrl)][6]=(unsigned char)(g);
					mask[(unsigned char)(cntrl)][7]=(unsigned char)(h);
					infilstate=3;
					break;
				}
				tmp[i]=rd; i++;
				break;
			case 5:
				break;
		}
	}
	fclose(infil);

	infil=fopen("counter_tx81z.cnt","r");
	if (infil==NULL) {
		printf("No counterfile, will number .syx files from zero.\n");
		syxfilecounter=0;
	} else {
		fscanf(infil, "%d", &syxfilecounter);
		fclose(infil);
	}
	fd = open(device,O_RDONLY, 0);
	wd = open(device,O_WRONLY, 0);

	if (fd < 0) {
		printf("could not open device: "); printf("%s", device); printf("\n");
		return -1;
	}

	i=0; mps=0;

	printf("Mapping setup:\n\n         0    1    2    3    4    5    6    7");
	for (i=0;i<128;i++) {
		if (mapped[i][0]!=0xfd) {
			printf("\nC %02X >  ",i);
			for (ii=0;ii<8;ii++)
				if (mapped[i][ii]==0xfd) {
					printf("     ");
				} else {
					printf("P%02X  ", mapped[i][ii]);
				}
		}
	}
	printf("\n\n[0xFF = Page, 0xFE = save syx]\n\n");

	page=0;

	while(1) {
		i++;
		x = read(fd, &inbuf, sizeof(inbuf) );

		switch(mps) {
			case 0: // normal
				if (inbuf == (0xB0+channel)) {
					mps=1;
				} else {
					write(wd, &inbuf, sizeof(inbuf));
				}
				break;
				
			case 1: // got ctrl
				controller = inbuf;
				mps=2;
				break;

			case 2: // get value
				if (mapped[controller][page]==0xFF) { // Change Page!
					if (page!=inbuf/(0x7F/noofpages)) {
						page=inbuf/(0x7F/noofpages);
						printf("Page = %d\n",page);
					}
					mps=0;
					break;
				}
				if (mapped[controller][page]==0xFE) { // Save SYX file
					notallset=0;
					for (i=0;i<256;i++) 
						if (parameterstatus[i]==0) { notallset=1; printf("%x ",i); }
					if (notallset==1) {
						printf("\nWarning not all parameters has been set!\nNo file saved!\n");
						mps=0;
						break;
					}
					// write syx file! Bulk dump one voice
					syxoutblock[0]=0xF0;
					syxoutblock[1]=0x43; //Yamaha
					syxoutblock[2]=channel;
					syxoutblock[3]=0x7e; // ACED
					syxoutblock[4]=0x21; // ACED
					cs=0;
					for (i=0x80;i<0x97;i++)
						cs+=syxoutblock[ii-0x7d]= parametervalue[i]; 
					syxoutblock[27]=(0x7f&cs); // checksum
					syxoutblock[28]=0xF7;
					syxoutblock[29]='\0';
					
					syxoutblock[30]=0xF0;
					syxoutblock[31]=0x43; //Yamaha
					syxoutblock[32]=channel;
					syxoutblock[33]=0x03; // VCED
					syxoutblock[34]=0x5d; // VCED
					cs=0;
					for (i=0x00;i<94;i++)
						cs+=syxoutblock[i+35]= parametervalue[i]; 
					syxoutblock[128]=(0x7f&cs); // checksum
					syxoutblock[129]=0xF7;
					syxoutblock[130]='\0';

					sprintf(tmp, "TX81Z%05d.syx", syxfilecounter);
					for (i=0 ; i<10 ; i++) //set the patchname
						syxoutblock[77+35+i]=tmp[i];

					utfil = fopen(tmp,"w");
					printf("Writing SYX file: %s\n",tmp);

					for (ii=0;ii<130;ii++) {
						fputc(syxoutblock[ii],utfil);
					}

					fclose(utfil);
					syxfilecounter++;
					utfil = fopen("counter.cnt","w");
					fprintf(utfil, "%d\n",syxfilecounter);
					fclose(utfil);
					printf("File write completed\n");
					mps=0;
					break;
				}
				if (mapped[controller][page]!=0xFD) { // Output parameter set sysex
					utbuf=0xF0; // sysex
					write(wd, &utbuf, sizeof(utbuf));
					
					utbuf=0x43; // Yamaha
					write(wd, &utbuf, sizeof(utbuf));
					
					utbuf=0x10+channel; // basic channel
					write(wd, &utbuf, sizeof(utbuf));
					
					if (mapped[controller][page] & 0x80) // ACED
						utbuf=0x13; // ACED (TX81z)
					else
						utbuf=0x12; // VCED (DX21 27 100)
 					write(wd, &utbuf, sizeof(utbuf));
 					
					utbuf=mapped[controller][page]; // Parameter
 					write(wd, &utbuf, sizeof(utbuf));
 					
					utbuf = parametervalue[mapped[controller][page]] = (unsigned char)( (long)(inbuf) * (long)(mask[controller][page]) / 0x7f); // Data scaled with mask
					write(wd, &utbuf, sizeof(utbuf));
					
					utbuf=0xF7; // end of sysex
					write(wd, &utbuf, sizeof(utbuf));
					
					printf("[C%02X > P%02X V%02X]\n", controller, mapped[controller][page], parametervalue[mapped[controller][page]]);
					parameterstatus[mapped[controller][page]]=1;
				} else {
					utbuf=0xB0+channel;
					write(wd, &utbuf, sizeof(utbuf));
					write(wd, &controller, sizeof(controller));
					write(wd, &inbuf, sizeof(inbuf));
					printf("[C%02X]\n",controller);
				}
				mps=0;
				break;
			default:
				mps=0;
		}
	}
	return 0;
}
