// DIB.cpp: implementation of the DIB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DIB.h"
#include"math.h"
#define WIDTHBYTES(bits)  ((bits+31)/32*4)
#define RECTWIDTH(x) (x->right-x->left)
#define RECTHEIGHT(x) (x->bottom-x->top)
#define THRESHOLDCONTRAST  40
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#define PI 3.1415926
extern int locax,locay;
#define m_WIDTH 600
#define m_HEIGHT 600

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
/////////////////////////////////////////////////////////////////////
HDIB DIB::ReadDIBFile(HANDLE hFile)
{
	BITMAPFILEHEADER bmfHeader;
	DWORD dwBitsSize;
	HANDLE hDIB;
	HANDLE hDIBtmp;
	LPBITMAPINFOHEADER lpbi;
	DWORD dwRead;
    //�õ��ļ���С
	dwBitsSize = GetFileSize(hFile,NULL);
	hDIB =  GlobalAlloc(GMEM_MOVEABLE,(DWORD)(sizeof(BITMAPINFOHEADER)));

	if(!hDIB)
		return NULL;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	if(!lpbi)
	{
		GlobalFree(hDIB);
		return NULL;
	}
	
	if(!ReadFile(hFile,(LPBYTE)&bmfHeader,sizeof(BITMAPFILEHEADER),&dwRead,NULL))
		goto ErrExit;
	if(sizeof(BITMAPFILEHEADER)!=dwRead)//��ȡ�ļ�����
		goto ErrExit;
	if(bmfHeader.bfType != 0x4d42)//�ļ����Ͳ�ƥ��
		goto ErrExit;
	if(!ReadFile(hFile,(LPBYTE)lpbi,sizeof(BITMAPINFOHEADER),&dwRead,NULL))
		goto ErrExit;
	if(sizeof(BITMAPINFOHEADER)!= dwRead)//��ȡ���ݳ���
		goto ErrExit;
	
	GlobalUnlock(hDIB);
	if(lpbi->biSizeImage==0)
		lpbi->biSizeImage = (this->BytePerLine(hDIB))*lpbi->biHeight;
	hDIBtmp = GlobalReAlloc(hDIB,lpbi->biSize+lpbi->biSizeImage,0);
	if(!hDIBtmp)
		goto ErrExitNoUnlock;
	else
		hDIB = hDIBtmp;
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	//��������趨�ļ�ָ��
	if(bmfHeader.bfOffBits != 0L)
		SetFilePointer(hFile,bmfHeader.bfOffBits,NULL,FILE_BEGIN);
    //��ȡ�ļ���������ɫ����
	if(ReadFile(hFile,(LPBYTE)lpbi+lpbi->biSize,lpbi->biSizeImage,&dwRead,NULL))
			goto OKExit;
	
	ErrExit:
		GlobalUnlock(hDIB);
	
	ErrExitNoUnlock:
		GlobalFree(hDIB); //�ͷ��ڴ�
		return NULL;

	OKExit:
		GlobalUnlock(hDIB);
		return hDIB;
		
}

HDIB DIB::LoadDIB(LPCTSTR lpFileName)
{
	HANDLE hDIB;
	HANDLE hFile;
	//�����ļ����
	if((hFile = CreateFile(lpFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL))!= INVALID_HANDLE_VALUE)
	{  
		//��ȡ����
		hDIB = ReadDIBFile(hFile);
		//�ر��ļ����
		CloseHandle(hFile);
		return hDIB;
	}
	return NULL;
}

BOOL DIB::PaintDIBTrue(HDC hDC,LPRECT lpDCRect,HANDLE hDIB,LPRECT lpDIBRect ,DWORD dwRop)
{
	LPBYTE lpDIBHdr;
	LPBYTE lpDIBBits;
	BOOL bSuccess = FALSE;

	if(!hDIB)
		return FALSE;
	lpDIBHdr = (LPBYTE)GlobalLock(hDIB);
	lpDIBBits = lpDIBHdr + sizeof(BITMAPINFOHEADER);
	bSuccess = StretchDIBits(hDC,lpDCRect->left,
								 lpDCRect->top,
								 RECTWIDTH(lpDCRect),
								 RECTHEIGHT(lpDCRect),
								 lpDIBRect->left,
								 ((LPBITMAPINFOHEADER)lpDIBHdr)->biHeight-lpDIBRect->top-RECTHEIGHT(lpDIBRect),
								 RECTWIDTH(lpDIBRect),
								 RECTHEIGHT(lpDIBRect),
								 lpDIBBits,
								 (LPBITMAPINFO)lpDIBHdr,
								 DIB_RGB_COLORS,
								 SRCCOPY);
	GlobalUnlock(hDIB);
	return bSuccess;
}

WORD DIB::BytePerLine(HANDLE hDIB)
{	
	WORD i;
	LPBITMAPINFOHEADER lpbi;
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	i = WIDTHBYTES((lpbi->biWidth)*24);
	GlobalUnlock(hDIB);
	return i;	
}


//����ʵ��ͼƬ�Ӳ�ɫ���ڰ׵�ת��
HDIB DIB::ToGray(HANDLE hDIB)
{
	HDIB hNewDIB = NULL;
	LPBITMAPINFOHEADER lpSrc,lpDest;
	LPBYTE lpS,lpD;
	DWORD dwBytesPerLine;
	DWORD dwImgSize;
	WORD wBytesPerLine;
	unsigned i ,j,height,width;
	if(!hDIB)
		return NULL;
	
	lpSrc = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

	dwBytesPerLine = WIDTHBYTES(24*(lpSrc->biWidth));
	dwImgSize = lpSrc->biHeight * dwBytesPerLine;
	//�����µ��ڴ棬��С����ԭ��ͼ��Ĵ�С
	hNewDIB = GlobalAlloc(GHND,sizeof(BITMAPINFOHEADER)+dwImgSize);

	lpDest = (LPBITMAPINFOHEADER)GlobalLock(hNewDIB);
	//����ͼƬ�ĳ�����ɫ��ȵ���Ϣ
	memcpy((void*)lpDest,(void*)lpSrc,sizeof(BITMAPINFOHEADER));
	DWORD dwSBytesPerLine;
	dwSBytesPerLine = (24*(lpSrc->biWidth)+31)/32*4;
	height = lpDest->biHeight;
	width = lpDest->biWidth;
	lpS = (LPBYTE)lpSrc;
	wBytesPerLine = this->BytePerLine(hDIB);
	lpD = (LPBYTE)lpDest;	
	lpS = lpS + sizeof(BITMAPINFOHEADER);
	lpD = lpD + sizeof(BITMAPINFOHEADER);
	unsigned  r , g ,b,gray ;
	//ɨ������ͼƬ��ʵ�ֻҶȻ�
	for(i = 0 ;i<height; i++)
	{
		for(j = 0 ;j<(unsigned )lpDest->biWidth;j++)
		{
	        //���ԭ��ͼƬ����ɫֵ
			r = *(lpS++);
			g = *(lpS++);
			b  = *(lpS++);
			//����Ҷ�ֵ
			gray = (g*50+r*39+b*11)/100;
			//����Ҷ�ֵ��Ŀ��ͼƬ
			*(lpD++)=gray;
			*(lpD++) = gray;
			*(lpD++) = gray;

			
		}
		//�������ֽڶ�������
	unsigned  k ;
		for(k=0;k<dwSBytesPerLine-lpSrc->biWidth*3;k++)
		{
			lpS++;
			lpD++;
		}
		
	}

	GlobalUnlock(hDIB);
	GlobalUnlock(hNewDIB);
   	return hNewDIB;
	
	

}



LPBYTE  DIB::FindDIBBits(HANDLE hDIB)
{
	LPBYTE lpDIB,lpDIBtmp;
	LPBITMAPINFOHEADER lpbi;
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	lpDIBtmp = (LPBYTE)lpbi;
	lpDIB = lpDIBtmp + sizeof(BITMAPINFOHEADER);
	GlobalUnlock(hDIB);
	return lpDIB;
}

long DIB::PixelOffset(int i,int j,WORD wBytePerLine)
{
	long   Offset;
	Offset = i*wBytePerLine + j*3;
	return Offset;
}





int DIB::BOUND(int a ,int b ,int rgb)
{
	if(rgb<0)
		return BOUND(a,b,abs(rgb));
	if(rgb>b)
		return b;
	return rgb;
}




//ʵ��ͼƬ�ĺڰ׶�ֵ��
void DIB::WhiteBlack(HANDLE hDIB,unsigned n)
{

	LPBITMAPINFOHEADER  lpbi;
	LPBYTE				lpS;
	int					width,height;
	long				lOffset;
	WORD                wBytesPerLine;

	if(!hDIB)
		return ;
	wBytesPerLine = this->BytePerLine(hDIB);
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
    //�õ�ͼƬ�ĳ�����Ϣ
	width = lpbi->biWidth;
	height = lpbi->biHeight;

	lpS = (LPBYTE)lpbi;
	//lpsָ��������
	lpS = lpS + sizeof(BITMAPINFOHEADER);
  	//ɨ������ͼƬ��ʵ�ֶ�ֵ��
	for(int i = 0;i<height;i++)
		for(int j = 0 ;j<width;j++)
		{   //�õ������������������е�ƫ��
			lOffset = this->PixelOffset(i,j,wBytesPerLine);
			if(*(lpS+lOffset)<n)//����ֵС���ٽ�ֵ
			{   //���������Ϊ��ɫ
				*(lpS+lOffset++) = 0;
				*(lpS+lOffset++) = 0;
				*(lpS+lOffset)   = 0;
			}
			else //����ֵ�����ٽ�ֵ
			{   
				//���������Ϊ��ɫ
				*(lpS+lOffset++) = 255;
				*(lpS+lOffset++) = 255;
				*(lpS+lOffset)   = 255;
			}
		}

		GlobalUnlock(hDIB);
		
}



DIB::DIB()
{	
	for(int i=0;i<ImgRange; i++)
		for (int j=0; j<ImgRange; j++)
			this->lab[i][j] = false;

}
DIB::~DIB()
{

}





BOOL DIB:: SaveDIB(HANDLE hDib, CFile& file)
{
	// Bitmap�ļ�ͷ
	BITMAPFILEHEADER bmfHdr;
	
	// ָ��BITMAPINFOHEADER��ָ��
	LPBITMAPINFOHEADER lpBI;
	
	// DIB��С
	DWORD dwDIBSize =0;

	if (hDib == NULL)
	{
		// ���DIBΪ�գ�����FALSE
		return FALSE;
	}

	// ��ȡBITMAPINFO�ṹ��������
	lpBI = (LPBITMAPINFOHEADER) ::GlobalLock((HGLOBAL) hDib);
	
	if (lpBI == NULL)
	{
		// Ϊ�գ�����FALSE
		return FALSE;
	}
	
	// �ж��Ƿ���WIN3.0 DIB
//	if (!IS_WIN30_DIB(lpBI))
//	{
		// ��֧���������͵�DIB����
		
		// �������
	//	::GlobalUnlock((HGLOBAL) hDib);
		
		// ����FALSE
	//	return FALSE;
//	}

	// ����ļ�ͷ

	// �ļ�����"BM"
	bmfHdr.bfType =  0x4d42; //DIB_HEADER_MARKER;

	// ����DIB��Сʱ����򵥵ķ����ǵ���GlobalSize()����������ȫ���ڴ��С��
	// ����DIB�����Ĵ�С�������Ƕ༸���ֽڡ���������Ҫ����һ��DIB����ʵ��С��
	
	// �ļ�ͷ��С����ɫ���С
	// ��BITMAPINFOHEADER��BITMAPCOREHEADER�ṹ�ĵ�һ��DWORD���Ǹýṹ�Ĵ�С��
//	dwDIBSize = *(LPDWORD)lpBI; //+ ::PaletteSize((LPSTR)lpBI);
	dwDIBSize = sizeof(BITMAPINFOHEADER);//+lpBI->biSizeImage;	
	// ����ͼ���С
	if ((lpBI->biCompression == BI_RLE8) || (lpBI->biCompression == BI_RLE4))
	{
		// ����RLEλͼ��û�������С��ֻ������biSizeImage�ڵ�ֵ
		dwDIBSize += lpBI->biSizeImage;
	}
	else
	{
		// ���صĴ�С
		DWORD dwBmBitsSize;

		// ��СΪWidth * Height
		dwBmBitsSize = WIDTHBYTES((lpBI->biWidth)*24) * lpBI->biHeight;
		
		// �����DIB�����Ĵ�С
		dwDIBSize += dwBmBitsSize;

		// ����biSizeImage���ܶ�BMP�ļ�ͷ��biSizeImage��ֵ�Ǵ���ģ�
		lpBI->biSizeImage = dwBmBitsSize;
	}


	// �����ļ���С��DIB��С��BITMAPFILEHEADER�ṹ��С
	bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER);
	
	// ����������
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;

	// ����ƫ����bfOffBits�����Ĵ�СΪBitmap�ļ�ͷ��С��DIBͷ��С����ɫ���С
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + lpBI->biSize;
											 // + PaletteSize((LPSTR)lpBI);
	// ����д�ļ�
//	TRY
	{
		// д�ļ�ͷ
		file.Write((LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER));
		
		// дDIBͷ������
		file.WriteHuge(lpBI, dwDIBSize);
	}
//	CATCH (CFileException, e)
//	{
		// �������
	//	::GlobalUnlock((HGLOBAL) hDib);
		
		// �׳��쳣
///		THROW_LAST();
//	}
//	END_CATCH
	
	// �������
	::GlobalUnlock((HGLOBAL) hDib);
	
	// ����TRUE
	return TRUE;
}


HANDLE DIB::CopyHandle( HANDLE hSrc)
{	
	HANDLE hDst;
	LPBITMAPINFOHEADER lpbi;
	int width,height;
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hSrc);
	width = lpbi->biWidth;
	height = lpbi->biHeight;
	hDst = GlobalAlloc(GMEM_MOVEABLE,lpbi->biSize+lpbi->biSizeImage);
	if(!hDst)
		return NULL;
	LPBYTE lpDest;
	lpDest = (LPBYTE)GlobalLock(hDst);
	memcpy(lpDest,(LPBYTE)lpbi,lpbi->biSize+lpbi->biSizeImage);
	GlobalUnlock(hSrc);
	GlobalUnlock(hDst);
	return hDst;

}


//����Ѱ��ͼƬ�е�������������ĵ�



#define THRESHOLD (RADIUS*2+1)*(RADIUS*2+1)*15
//������һ��ͼƬ��Ѱ��ƥ������ĵ�
BOOL DIB::MatchImportantPoint(HANDLE hDIB,int CharaterInfo[RADIUS*2+1][RADIUS*2+1][3],CPoint *ImPoint)
{
	LPBITMAPINFOHEADER lpbi;
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	int width = lpbi->biWidth;
	int height = lpbi->biHeight;
	LPBYTE lpData = this->FindDIBBits(hDIB);
    WORD wBytesPerLine = this->BytePerLine(hDIB);
	long lOffset;
	long sum =100000,tempsum;
	//ɨ������ͼƬ����Ե�㣩����
	for(int i=RADIUS ;i<height-RADIUS;i++)
		for(int j=RADIUS;j<width-RADIUS;j++)
		{	
			tempsum =0;
			//ɨ����RADIUS*2+1Ϊ�߳�������������
			for(int k=-RADIUS;k<=RADIUS;k++)
				for(int kk=-RADIUS;kk<=RADIUS;kk++)
				{
					//���㵱ǰ�����κ���֪�����������ɫ��ֵ

				lOffset = this->PixelOffset(i+k,j+kk,wBytesPerLine);
				int colorblue = abs(*(lpData+lOffset++)-CharaterInfo[k+RADIUS][kk+RADIUS][0]);
				int colorgreen = abs(*(lpData+lOffset++)-CharaterInfo[k+RADIUS][kk+RADIUS][1]);
				int colorred = abs(*(lpData+lOffset++)-CharaterInfo[k+RADIUS][kk+RADIUS][2]);
				tempsum +=colorgreen+colorblue+colorred;
				}
				if(tempsum<sum)
				{  //���²�ֵ
					sum = tempsum;
					//�������������
					ImPoint->x = j;
					ImPoint->y = i;
				}
		}

		if(sum <THRESHOLD){//�ҵ���������������
		//����Ĵ�����ҵ�������ı߿����ó�Ϊ��ɫ
		for(i =-RADIUS;i<=RADIUS;i++)
		{
			lOffset = this->PixelOffset(ImPoint->y-RADIUS,ImPoint->x+i,wBytesPerLine);
			*(lpData+lOffset++) = 255;
			*(lpData+lOffset++) = 255;
			*(lpData+lOffset++) = 255;

		}

		for(i =-RADIUS;i<=RADIUS;i++)
		{
			lOffset = this->PixelOffset(ImPoint->y+RADIUS,ImPoint->x+i,wBytesPerLine);
			*(lpData+lOffset++) = 255;
			*(lpData+lOffset++) = 255;
			*(lpData+lOffset++) = 255;

		}
		for(i =-RADIUS;i<=RADIUS;i++)
		{
			lOffset = this->PixelOffset(ImPoint->y+i,ImPoint->x+RADIUS,wBytesPerLine);
			*(lpData+lOffset++) = 255;
			*(lpData+lOffset++) = 255;
			*(lpData+lOffset++) = 255;

		}
		for(i =-RADIUS;i<=RADIUS;i++)
		{
			lOffset = this->PixelOffset(ImPoint->y+i,ImPoint->x-RADIUS,wBytesPerLine);
			*(lpData+lOffset++) = 255;
			*(lpData+lOffset++) = 255;
			*(lpData+lOffset++) = 255;

		}
		GlobalUnlock(hDIB);
		return true;
		}
		else AfxMessageBox("Can't find the corresponding point!");
	GlobalUnlock(hDIB);
	return false;
}
//�Ƚ�����ͼƬ�����ƶ�




BOOL DIB::IsScaterPoint(int x, int y, int width, int height, LPBYTE lpData,WORD wBytesPerLine,  int threshold,bool lab[m_HEIGHT][m_WIDTH])
{
	long lOffset;
	//�õ����ݵ�ƫ��
	lOffset = this->PixelOffset(y,x,wBytesPerLine);
	//�жϸõ��Ƿ�Ϊ��ɫ�Լ��Ƿ�������
	if(*(lpData+lOffset) == 255 && lab[y][x] == false)
	{	
		//�����ȼ�һ
		this->lenth++;
		//���ı�־λ
		lab[y][x] = true;
		//��������ȴﵽ�ٽ�ֵ�򷵻���
	if(this->lenth >= threshold)
		return true;
	//���ұߵ�ı߽��ж��Լ���־λ�ж�
	if(x+1<width && lab[y][x+1] == false)
	{	
		//�ݹ���ñ����������ұߵĵ�����ж�
		IsScaterPoint(x+1,y,width,height,lpData,wBytesPerLine,threshold,lab);
		if(this->lenth>=threshold)
			return true;
		
	}
	//������ߵĵ�
	if(x-1>=0 && lab[y][x-1] == false)
	{
		(IsScaterPoint(x-1,y,width,height,lpData,wBytesPerLine,threshold,lab));
		if(this->lenth>=threshold)
			return true;
		
	}
	//��������ĵ�
	if(y-1>=0 && lab[y-1][x]==false)
	{
		(IsScaterPoint(x,y-1,width,height,lpData,wBytesPerLine,threshold,lab));
		if(this->lenth>=threshold)
			return true;
		
	}
	//��������ĵ�
	if(y+1<height && lab[y+1][x]==false)
	{	(IsScaterPoint(x,y+1,width,height,lpData,wBytesPerLine,threshold,lab));
			if(this->lenth>=threshold)
			return true;
			
	}
	//�������µĵ�
	if(y+1<height  && x+1 <width && lab[y+1][x+1]==false)
	{	(IsScaterPoint(x+1,y+1,width,height,lpData,wBytesPerLine,threshold,lab));
			if(this->lenth>=threshold)
			return true;
			
	}
	//�������µĵ�
	if(y+1<height && x-1 >=0 && lab[y+1][x-1]==false)
	{	(IsScaterPoint(x-1,y+1,width,height,lpData,wBytesPerLine,threshold,lab));
			if(this->lenth>=threshold)
			return true;
			
	}
	//�������ϵĵ�
	if(y-1>=0 && x-1 >=0 &&lab[y-1][x-1]==false)
	{	(IsScaterPoint(x-1,y-1,width,height,lpData,wBytesPerLine,threshold,lab));
			if(this->lenth>=threshold)
			return true;
			
	}
	//�������ϵĵ�
	if(y-1<height && x+1<width && lab[y+1][x]==false)
	{	(IsScaterPoint(x+1,y-1,width,height,lpData,wBytesPerLine,threshold,lab));
			if(this->lenth>=threshold)
			return true;
			
	}
	}	
		//����ݹ���������ȴﲻ���ٽ�ֵ�����ؼ�
		return false;
}



BOOL DIB::LightingCompensate(HANDLE hDIB)
{
	if(!hDIB)
		return FALSE;
	LPBITMAPINFOHEADER lpbi;
	int width,height;
	LPBYTE lpData;
	WORD wBytesPerLine;
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	//�õ�ͼƬ��͸�
	width = lpbi->biWidth;
	height = lpbi->biHeight;
	//�õ�ͼƬ������
	lpData = this->FindDIBBits(hDIB);
	//�õ�ͼƬÿ�е�������ռ�ֽڸ���
	wBytesPerLine = this->BytePerLine(hDIB);
	//����ϵ��
	const float thresholdco = 0.05;
	//���ظ������ٽ糣��
	const int thresholdnum = 100;
	//�Ҷȼ�����
	int histogram[256];
	for(int i =0;i<256;i++)
		histogram[i] = 0;
	//���ڹ���С��ͼƬ���ж�
	if(width*height*thresholdco < thresholdnum)
		return false;
	int colorr,colorg,colorb;
	long lOffset;
	//��������ͼƬ
	for( i=0;i<height;i++)
		for(int j=0;j<width;j++)
		{	
			//�õ��������ݵ�ƫ��
			lOffset = this->PixelOffset(i,j,wBytesPerLine);
			//�õ�rgbֵ
			colorb = *(lpData+lOffset++);
			colorg = *(lpData+lOffset++);
			colorr = *(lpData+lOffset++);
			//����Ҷ�ֵ
			int gray = (colorr * 299 + colorg * 587 + colorb * 114)/1000;
			histogram[gray]++;
		}
		int calnum =0;
		int total = width*height;
		int num;
		//�����ѭ���õ�����ϵ��thresholdco���ٽ�Ҷȼ�
		for(i =0;i<256;i++)
		{
			if((float)calnum/total < thresholdco)
			{
				calnum+= histogram[255-i];
				num = i;
			}
			else
				break;
		}
		int averagegray = 0;
		calnum =0;
		//�õ����������������ܵĻҶ�ֵ
		for(i = 255;i>=255-num;i--)
		{
			averagegray += histogram[i]*i;
			calnum += histogram[i];
		}
		averagegray /=calnum;
		//�õ����߲�����ϵ��
		float co = 255.0/(float)averagegray;
		//�����ѭ����ͼ����й��߲���
		for(i =0;i<height;i++)
			for(int j=0;j<width;j++)
			{	
				//�õ����ݱ���
				lOffset = this->PixelOffset(i,j,wBytesPerLine);
				//�õ���ɫ����
				colorb = *(lpData+lOffset);
				//����
				colorb *=co;
				//�ٽ��ж�
				if(colorb >255)
					colorb = 255;
				//����
				*(lpData+lOffset) = colorb;
				//��ɫ����
				colorb = *(lpData+lOffset+1);
				colorb *=co;
				if(colorb >255)
					colorb = 255;
				*(lpData+lOffset+1) = colorb;
				//��ɫ����
				colorb = *(lpData+lOffset+2);
				colorb *=co;
				if(colorb >255)
					colorb = 255;
				*(lpData+lOffset+2) = colorb;

			}
			return TRUE;
}

BOOL DIB::FaceModeling(int Cr,int Cb)
{	
	//Cb��ϵ������
	const float cx = 114.38;
	//cr��ϵ������
	const float cy = 160.02;
	//�Ƕȳ���
	const float theta = 2.53;
	//x���ߺ�y���ߵ���������
	const float ecx = 1.60;
	const float ecy = 2.41;
	//����
	const float a = 25.39;
	//����
	const float b = 14.03;
	//���ƶȳ���
	const float judge = 0.5;
	//����õ�x����ֵ
	float  x = cos(theta)*(Cb-cx)+sin(theta)*(Cr-cy);
	//y����ֵ
	float  y = -sin(theta)*(Cb -cx)+cos(theta)*(Cr-cy);
	//����������
	float temp = pow(x-ecx,2)/pow(a,2)+pow(y-ecy,2)/pow(b,2);
	//�������Ҫ�󷵻��棬�����
	if(fabs(temp-1.0)<judge)
		return TRUE;
	else
		return FALSE;

}

LPBYTE DIB::YcctoRgb(LPBYTE lpYcc,WORD wBytesPerLine,int height,int width)
{
	LPBYTE lpRGB;
	//�����ڴ�
	lpRGB = new BYTE[wBytesPerLine*height];
	//������ݷ���
	if(lpRGB == NULL)
	{
		AfxMessageBox("not enought memory");
		return NULL;
	}
	long lOffset;
	//�����ѭ��ʵ�ִ�ycc��rgb��ת��
	for(int i=0;i<height;i++)
		for(int j=0;j<width;j++)
		{	
			//�õ����ݱ���
			lOffset = PixelOffset(i,j,wBytesPerLine);
			//�õ�y��Cr��Cb����ֵ
			int Y = *(lpYcc+lOffset);
			int Cr = *(lpYcc+lOffset+1);
			int Cb = *(lpYcc+lOffset+2);
			//���ù�ʽ���м��㣬���ѽ�����浽��̬��������
			*(lpRGB+lOffset+2) = (1164*(Y-16)+1596*(Cr-128))/1000;
			*(lpRGB+lOffset+1) = (1164*(Y-16) - 813*(Cr-128) - 392*(Cb-128))/1000;
			*(lpRGB+lOffset)   = (1164*(Y-16) +2017*(Cb-128))/1000;
		}
	return lpRGB;
}


int DIB::_Cb(int Y)
{	
	int Cb;
	//������Ⱥ�С�����
	if(Y<Kl)
		Cb = 108 + ((Kl-Y)*10)/(Kl-Ymin);
	//���Ⱥܴ�����
	else if(Y>Kh)
		Cb = 108 + ((Y-Kh)*10)/(Ymax - Kh);
	else 
		Cb = -1;
	return Cb;
}

int DIB::_Cr(int Y)
{
	int Cr;
	//���Ⱥ�С�����
	if(Y<Kl)
		Cr = 154 - ((Kl-Y)*10)/(Kl-Ymin);
	//���Ⱥܴ�����
	else if(Y>Kh)
		Cr = 154 - ((Y-Kh)*22)/(Ymax - Kh);
	else
		Cr = -1;
	return Cr;
}
int DIB::_WCr(int Y)
{
	int WCr;
	if(Y<Kl)
		//���Ⱥ�С�����
		WCr = WLcr + ((Y-Ymin)*(Wcr-WLcr))/(Kl-Ymin);
	else if(Y>Kh)
		//���Ⱥܴ�����
		WCr = WHcr + ((Ymax-Y)*(Wcr-WHcr))/(Ymax-Kh);
	else WCr = -1;
	return WCr;
}

int DIB:: _WCb(int Y)
{
	int WCb;
	if(Y<Kl)
		//���Ⱥ�С�����
		WCb = WLcb + ((Y-Ymin)*(Wcb-WLcb))/(Kl-Ymin);
	else if(Y>Kh)
		//���Ⱥܴ�����
		WCb = WHcb + ((Ymax-Y)*(Wcb-WHcb))/(Ymax-Kh);
	else WCb = -1;
	return WCb;
}
void DIB::YccTransform(LPBYTE lpYcc,WORD wBytesPerLine,int height,int width)
{	
	int Y,Cr,Cb;
	long lOffset;
	//�����ѭ��ʵ��yccɫ�ʿռ�ķ�����ת��
	for(int i=0;i<height;i++)
		for(int j=0;j<width;j++)
		{	
			//�õ�����ƫ��
			lOffset = PixelOffset(i,j,wBytesPerLine);
			//�õ�y��Cr��Cb��ֵ
			Y = *(lpYcc+lOffset);
			Cr = *(lpYcc+lOffset+1);
			Cb = *(lpYcc+lOffset+2);
			//���y��ֵ�������ٽ�ֵ֮�䣬���ֲ���
			if(Y>=Kl && Y<=Kh)
				continue;
			//���÷�����ת����������Cr��Cb����ֵ
			Cr = (Cr-_Cr(Y))*(Wcr/_WCr(Y))+_Cr(Kh);
			Cb = (Cb-_Cb(Y))*(Wcb/_WCb(Y))+_Cb(Kh);
			*(lpYcc+lOffset+1) = Cr;
			*(lpYcc+lOffset+2) = Cb;
		}
}

void DIB::faceear(LPBYTE lpYcc, WORD wBytesPerLine, int height,int width, bool flag[ImgRange][ImgRange])
{	
	//��ʼ����־λ
	for (int i=0; i<ImgRange; i++)
		for (int j=0; j<ImgRange; j++)
		{
			flag[i][j] = false;
		}

	long lOffset;
	int Cr;
	int Cb;
	for (i=0; i<height; i++)
		for (int j=0; j<width; j++)
		{	
			//�õ�ƫ��
			lOffset = PixelOffset(i,j,wBytesPerLine);
			//�õ�Cr��Cb��ֵ
			Cr = *(lpYcc+lOffset+1);
			Cb = *(lpYcc+lOffset+2);
			//������ɫ��ģ
			if(FaceModeling(Cr,Cb))
			{	
				//�޸ı�־λ
				flag[i][j] = true;
			}
		}
	
}

void  DIB::FaceLocate(HANDLE hDIB, CRect faceLocation[10], int &faceNum)
{	

	HANDLE hDIBTemp;
	//���浱ǰ����
	hDIBTemp = this->CopyHandle(hDIB);
	LPBITMAPINFOHEADER lpbi;
	LPBYTE lpData;
	WORD wBytesPerLine;
	int height;
	int width;
	long lOffset;
	//�õ�ͼ��Ļ�����Ϣ
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width  = lpbi->biWidth;
	lpData = FindDIBBits(hDIB);
	wBytesPerLine = BytePerLine(hDIB);

	//������Ŀ��ʼ��Ϊ0
	faceNum =0;
	for(int k=0; k<10; k++)
	{	
		//��ʼ������
		faceLocation[k].bottom = -1;
		faceLocation[k].top = height;
		faceLocation[k].right = -1;
		faceLocation[k].left = width;
	}

	for(int i=0; i<height; i++)
		for (int j=0; j<width; j++)
		{	
			//ƫ��
			lOffset = PixelOffset(i,j,wBytesPerLine);
			int num;
			//��ǰ�����ֵ
			num = *(lpData + lOffset);
			if (num !=0)//���Ǻ�ɫ
			{	
				//�ݹ����
				RecursiveLocateRect(lpData, wBytesPerLine, i, j, num, faceLocation[faceNum]);
				faceNum++;
			}
		}
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);
	//��ֵ��ԭ
	hDIB = this->CopyHandle(hDIBTemp);
	lpData = FindDIBBits(hDIB);
	wBytesPerLine = BytePerLine(hDIB);
	for (i=0; i<faceNum; i++)
		for (int j=faceLocation[i].top; j<faceLocation[i].bottom; j++)
		{	
			//�ѵõ���������������ɫ���α�ע��������ֱ��������
			lOffset = this->PixelOffset(j, faceLocation[i].left, wBytesPerLine);
			*(lpData + lOffset++) = 0;
			*(lpData + lOffset++) = 255;
			*(lpData + lOffset++) = 0;
			lOffset = this->PixelOffset(j, faceLocation[i].right, wBytesPerLine);
			*(lpData + lOffset++) = 0;
			*(lpData + lOffset++) = 255;
			*(lpData + lOffset++) = 0;

		}

		for (i=0; i<faceNum; i++)
		for (int j=faceLocation[i].left; j<faceLocation[i].right; j++)
		{	
			//����ˮƽ��������α�
			lOffset = this->PixelOffset(faceLocation[i].top, j, wBytesPerLine);
			*(lpData + lOffset++) = 0;
			*(lpData + lOffset++) = 255;
			*(lpData + lOffset++) = 0;
			lOffset = this->PixelOffset(faceLocation[i].bottom, j, wBytesPerLine);
			*(lpData + lOffset++) = 0;
			*(lpData + lOffset++) = 255;
			*(lpData + lOffset++) = 0;

		}
	
		
	
	GlobalFree(hDIBTemp);	
	GlobalUnlock(hDIB);
}

void DIB::RecursiveLocateRect(LPBYTE lpData,WORD wBytesPerLine, int y, int x, int num, CRect &faceRect)
{	
	long lOffset;
	//�õ�ƫ��
	lOffset = PixelOffset(y,x,wBytesPerLine);
	//��ֵ�ж�
	if(*(lpData + lOffset) == num)
	{	
		//������ɫΪ��ɫ
		*(lpData + lOffset++) = 0;
		*(lpData + lOffset++) = 0;
		*(lpData + lOffset++) = 0;
		//�޸ľ��ε����������ĸ���λ��
		if(faceRect.bottom < y)
		{
			faceRect.bottom = y;
		}

		if(faceRect.top > y)
		{
			faceRect.top = y;
		}

		if(faceRect.right < x)
		{
			faceRect.right = x;
		}

		if(faceRect.left > x)
		{
			faceRect.left = x;
		}
		//�������ҵ��ñ��������������ж�
		RecursiveLocateRect(lpData, wBytesPerLine, y-1, x, num,faceRect);
		RecursiveLocateRect(lpData, wBytesPerLine, y+1, x, num, faceRect);
		RecursiveLocateRect(lpData, wBytesPerLine, y, x-1, num, faceRect);
		RecursiveLocateRect(lpData, wBytesPerLine, y, x+1, num, faceRect);
	}
	
}

void DIB::EyeMapC(LPBYTE lpRgb, const LPBYTE lpYcc,  WORD wBytesPerLine, CRect faceLocation)
{
	long lOffset;
	int cr;
	int cb;
	//���ݴ������ľ�����������۾���ɫ��ƥ��
	for(int i=faceLocation.top; i<=faceLocation.bottom; i++)
		for (int j=faceLocation.left; j<=faceLocation.right; j++)
		{	
			//�õ�Cr��Cb��ֵ
			lOffset = PixelOffset(i, j, wBytesPerLine);
			cr = *(lpYcc + lOffset +1);
			cb = *(lpYcc + lOffset +2);
			//��־
			bool lab;
			//�ж�Cb��������ֵ�����޸ı�־
			int cmap = cb -116 ;
			if(cmap >-1 && cmap <4)
				lab = true;
			else
				lab = false;
			//�ж�Cr��������ֵ�����޸ı�־
			 cmap =  cr- 144  ;
			if(cmap <=-2 || cmap>= 2)
			{
				lab = false;
				
			}
			//���ݱ�־�趨ͼ����ɫ
			if(lab)
				cmap = 255;
			else
				cmap = 0;
			//����ͼ����ɫ
			*(lpRgb + lOffset++) = cmap;
			*(lpRgb + lOffset++) = cmap;
			*(lpRgb + lOffset++) = cmap;
		}
	
}

void DIB::EyeMapb(LPBYTE lpRgb, const LPBYTE lpYcc,  WORD wBytesPerLine, CRect faceLocation)
{
	long lOffset;
	int cr;
	int cb;

	for(int i=faceLocation.top; i<=faceLocation.bottom; i++)
		for (int j=faceLocation.left; j<=faceLocation.right; j++)
		{
			lOffset = PixelOffset(i, j, wBytesPerLine);
			cb = *(lpYcc + lOffset +2);
		

			*(lpRgb + lOffset++) = cb;
			*(lpRgb + lOffset++) = cb;
			*(lpRgb + lOffset++) = cb;
		}
	
}

void DIB::EyeMapR(LPBYTE lpRgb, const LPBYTE lpYcc,  WORD wBytesPerLine, CRect faceLocation)
{
	long lOffset;
	int cr;
	int cb;

	for(int i=faceLocation.top; i<=faceLocation.bottom; i++)
		for (int j=faceLocation.left; j<=faceLocation.right; j++)
		{
			lOffset = PixelOffset(i, j, wBytesPerLine);
			cr = *(lpYcc + lOffset +1);
			cb = *(lpYcc + lOffset +2);
		
			
			*(lpRgb + lOffset++) = cr;
			*(lpRgb + lOffset++) = cr;
			*(lpRgb + lOffset++) = cr;
		}
	
}

void DIB::ErasionFalseArea(HANDLE hDIB)
{
	int PixelNum[255];
	LPBITMAPINFOHEADER lpbi;
	int width;
	int height;
	LPBYTE lpData;
	WORD wBytesPerLine;
	long lOffset;
	//�õ�������Ϣ
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width  = lpbi->biWidth;
	//�õ�������ָ���ÿ���ֽ���
	lpData = FindDIBBits(hDIB);
	wBytesPerLine = BytePerLine(hDIB);
	//��ʼ�������ۼ�����
	for (int i=0; i<255; i++)
	{
		PixelNum[i] = 0;
	}
	
	int calNum =1;
	for (i=0; i<height; i++)
		for (int j=0; j<width; j++)
		{
			lOffset = PixelOffset(i,j,wBytesPerLine);
			//�������Ϊ��ɫ
			if (*(lpData + lOffset)==255)
			{	
				//�ݹ�ͳ�Ƹ����������İ�ɫ�����ص����
				RecursiveCal(lpData, i,j,wBytesPerLine, PixelNum[calNum],calNum);
				calNum++;
			}
		}
		
		for (i=0; i<calNum; i++)
		{	
			//������ص����С��һ����Ŀ��������־����Ϊ0
			if (PixelNum[i] < AREAPIXEL)
			{
				PixelNum[i] = 0;
			}
		}
		//�����ѭ�����ݱ�־�����������趨ͼ�����ɫ
		for(i=0; i<height; i++)
			for (int j=0; j<width; j++)
			{
				lOffset = PixelOffset( i,j,wBytesPerLine);
				int num = *(lpData + lOffset);
				//�����ǰ�㲻�Ǻ�ɫ��
				if(num != 0)
				{	
					//�����־����Ϊ0��������Ϊ��ɫ
					if(PixelNum[num] == 0)
					{
						*(lpData+lOffset++) =0;
						*(lpData+lOffset++) =0;
						*(lpData+lOffset++) =0;
					}
					//��������Ϊ��ɫ
					else
					{
						*(lpData+lOffset++) =255;
						*(lpData+lOffset++) =255;
						*(lpData+lOffset++) =255;
					}
				}
			}
}


void DIB::RecursiveCal(LPBYTE lpData, int y, int x, WORD wBytesPerLine, int &pixelNum, int num)
{	
	long lOffset;
	
	lOffset = PixelOffset(y,x,wBytesPerLine);
	//�����ǰ��Ϊ��ɫ��
	if(*(lpData+lOffset) == 255)
	{	
		//�ѵ�ǰ���С���ó�Ϊ���ֵ
		*(lpData+lOffset++) = num;
		*(lpData+lOffset++) = num;
		*(lpData+lOffset++) = num;
		//���ظ�����һ
		pixelNum++;
	
	int tempx;
	int tempy;
	
	//�ݹ鵱ǰ������ĵ�
	tempy = y-1;
	tempx = x;
	RecursiveCal(lpData,tempy,tempx,wBytesPerLine,pixelNum,num);
	
	//����ĵ�
	tempy = y+1;
	tempx = x;
	RecursiveCal(lpData,tempy,tempx,wBytesPerLine,pixelNum,num);
	
	//��ߵĵ�
	tempy = y;
	tempx = x-1;
	RecursiveCal(lpData,tempy,tempx,wBytesPerLine,pixelNum,num);
	//�ұߵĵ�
	tempy = y;
	tempx = x+1;
	RecursiveCal(lpData,tempy,tempx,wBytesPerLine,pixelNum,num);
	
	}

}

void DIB::eyeMap(LPBYTE lpResult, bool eyemapc[][ImgRange], bool eyemapl[][ImgRange], bool lab[][ImgRange], WORD wBytesPerLine, CRect faceLocation)
{	
	long lOffset;
	//���ݵõ������Ⱥ�ɫ����Ϣ���۾������ۺ�ƥ��
	for(int i=faceLocation.top; i<=faceLocation.bottom; i++)
		for (int j=faceLocation.left; j<faceLocation.right; j++)
		{
			lOffset = PixelOffset(i, j, wBytesPerLine);
			//�����ǰ������Ⱥ�ɫ��ƥ�䶼Ϊ�棬����������������
			//������Ϊ��ɫ����������Ϊ��ɫ
			if((eyemapc[i][j]) && (eyemapl[i][j]) && lab[i][j])
			{
				*(lpResult + lOffset++) = 255;
				*(lpResult + lOffset++) = 255;
				*(lpResult + lOffset++) = 255;
			}

			else
			{
				*(lpResult + lOffset++) = 0;
				*(lpResult + lOffset++) = 0;
				*(lpResult + lOffset++) = 0;
			}
		}
}

void DIB::EyeMapL(LPBYTE lpRgb, WORD wBytesPerLine, CRect faceLocation)
{
	int r;
	int g;
	int b;
	int gray ;
	long lOffset;
	//�����ѭ��ʵ���۾�������ƥ��
	for (int i=faceLocation.top; i<=faceLocation.bottom; i++)
		for (int j=faceLocation.left; j<=faceLocation.right; j++)
		{
			lOffset = PixelOffset(i, j, wBytesPerLine);
			//�õ�rgbֵ
			b = *(lpRgb + lOffset);
			g = *(lpRgb + lOffset+1);
			r = *(lpRgb + lOffset+2);
			//����õ��Ҷ�
			gray = (r*10+g*30+b*60)/100;
			//�����۾��������������趨ͼ�����ֵ
			if(100<gray && 125>gray)
				gray =255;
			else
				gray = 0;
			*(lpRgb + lOffset++) = gray;
			*(lpRgb + lOffset++) = gray;
			*(lpRgb + lOffset++) = gray;
		}
}

void  DIB::RgbtoYcb(HANDLE hDIB, LPBYTE lpYcb)
{
	LPBITMAPINFOHEADER lpbi;
	int width,height;
	WORD wBytesPerLine;
	LPBYTE lpData;
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	//�õ�ͼ��Ļ�����Ϣ
	width = lpbi->biWidth;
	height = lpbi->biHeight;
	lpData = FindDIBBits(hDIB);
	wBytesPerLine = BytePerLine(hDIB);

	long lOffset;
	//�����ѭ��ʵ�ִ�rgb��ycc��ת��
	for(int i=0;i<height;i++)
		for(int j=0;j<width;j++)
		{
				lOffset = PixelOffset(i,j,wBytesPerLine);
				//�õ�rgb��ֵ
				int b = *(lpData + lOffset);
				int g = *(lpData + lOffset+1);
				int r = *(lpData + lOffset+2);
				//����õ�y��cr��cb����ֵ
				int Y = (257*r+504*g+98*b)/1000+16;
				int Cr = (439*r-368*g-71*b)/1000+128;
				int Cb = (-148*r-291*g+439*b)/1000+128;
				//�������õ�����ֵ
				*(lpYcb+lOffset++) = Y;
				*(lpYcb+lOffset++) = Cr;
				*(lpYcb+lOffset++) = Cb;

		}
		GlobalUnlock(hDIB);

}

void DIB::Erasion(HANDLE hDIB)
{
	LPBITMAPINFOHEADER lpbi;
	LPBYTE lpData;
	WORD wBytesPerLine;
	long lOffset;
	long lOffsetJudge;
	int height;
	int width;
	
	//�õ���������
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width = lpbi->biWidth;
	wBytesPerLine = BytePerLine(hDIB);
	lpData = FindDIBBits(hDIB);
	
	HANDLE hTempDIB;
	LPBYTE lpTemp;
	//����ͬ����С���ڴ�
	hTempDIB =   GlobalAlloc(GMEM_MOVEABLE,(DWORD)(sizeof(BITMAPINFOHEADER) + wBytesPerLine*height));
	//�ж��ڴ����
	if(!hTempDIB)
	{	
		GlobalFree(hTempDIB);
		GlobalFree(hDIB);
		return;
	}
	lpTemp = (LPBYTE)GlobalLock(hTempDIB);
	lpTemp+= sizeof(BITMAPINFOHEADER);
	//�����ѭ��ʵ�ָ�ʴ����
	for (int i=1; i<height-1; i++)
		for (int j=1; j<width-1; j++)
		{
			lOffset = PixelOffset(i,j,wBytesPerLine);
			//���Ϊ��ɫ��
			if (*(lpData+lOffset) == 255)
			{	
				//��������ĵ�
				lOffsetJudge = PixelOffset(i-1, j, wBytesPerLine);
				//����Ǻ�ɫ�Ͱ�ԭ���ĵ�����Ϊ��ɫ��������ѭ��
				if (*(lpData + lOffsetJudge) ==0)
				{
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					continue;
				}
				//��������ĵ�
				lOffsetJudge = PixelOffset(i+1, j, wBytesPerLine);
				if (*(lpData + lOffsetJudge) ==0)
				{
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					continue;
				}
				//����ĵ�
				lOffsetJudge = PixelOffset(i, j-1, wBytesPerLine);
				if (*(lpData + lOffsetJudge) ==0)
				{
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					continue;
				}
				//����ĵ�
				lOffsetJudge = PixelOffset(i, j+1, wBytesPerLine);
				if (*(lpData + lOffsetJudge) ==0)
				{
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					continue;
				}
				//������������ĸ��㶼�ǰ�ɫ��������Ϊ��ɫ
				lOffset = this->PixelOffset(i, j, wBytesPerLine);
				*(lpTemp + lOffset)   = 255;
				*(lpTemp + lOffset+1) = 255;
				*(lpTemp + lOffset+2) = 255;
				
			}
			//�����ǰ��Ϊ��ɫ��������ʱ��Ŀ������������Ϊ��ɫ
			else
			{
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;
			}
		}
		
		//��ͼ���ܱߵĵ�ȫ������Ϊ��ɫ
		for(i=0; i<height; i++)
		{
			lOffset = PixelOffset(i, 0, wBytesPerLine);
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;

		}
		
		for(i=0; i<height; i++)
		{
			lOffset = PixelOffset(i, width-1, wBytesPerLine);
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;

		}

	for (i=0; i<width; i++)
		{
			lOffset = PixelOffset(0, i, wBytesPerLine);
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;

		}
	
	for (i=0; i<width; i++)
		{
			lOffset = PixelOffset(height-1, i, wBytesPerLine);
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;

		}
	//����ʱ�������ֵ������ԭ���ľ������
	memcpy(lpData,lpTemp,wBytesPerLine*height);
	GlobalUnlock(hDIB);
	GlobalUnlock(hTempDIB);
	GlobalFree(hTempDIB);

}


void DIB::Erasion2(HANDLE hDIB)
{
	LPBITMAPINFOHEADER lpbi;
	LPBYTE lpData;
	WORD wBytesPerLine;
	long lOffset;
	long lOffsetJudge;
	int height;
	int width;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width = lpbi->biWidth;
	wBytesPerLine = BytePerLine(hDIB);
	lpData = FindDIBBits(hDIB);
	
	HANDLE hTempDIB;
	LPBYTE lpTemp;
	//������ͬ��С���ڴ�
	hTempDIB =   GlobalAlloc(GMEM_MOVEABLE,(DWORD)(sizeof(BITMAPINFOHEADER) + wBytesPerLine*height));
	if(!hTempDIB)
	{	
		GlobalFree(hTempDIB);
		GlobalFree(hDIB);
		return;
	}
	lpTemp = (LPBYTE)GlobalLock(hTempDIB);
	lpTemp+= sizeof(BITMAPINFOHEADER);
	//����Ĵ���ʵ�ָ�ʴ����
	for (int i=1; i<height-1; i++)
		for (int j=1; j<width-1; j++)
		{	
			//�����ǰ��Ϊ��ɫ
			lOffset = PixelOffset(i,j,wBytesPerLine);
			if (*(lpData+lOffset) == 255)
			{
				
				//�ж���ߵĴ��㣬����Ǻ�ɫ�ľͰ���ʱ�����еĶ�Ӧ������Ϊ��ɫ
				lOffsetJudge = PixelOffset(i, j-1, wBytesPerLine);
				if (*(lpData + lOffsetJudge) ==0)
				{
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					continue;
				}
				//�����ұߵĵ�
				lOffsetJudge = PixelOffset(i, j+1, wBytesPerLine);
				if (*(lpData + lOffsetJudge) ==0)
				{
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					*(lpTemp + lOffset++) = 0;
					continue;
				}

				//����������ߵĵ㶼�ǰ�ɫ�ѵ�����Ϊ��ɫ
				lOffset = this->PixelOffset(i, j, wBytesPerLine);
				*(lpTemp + lOffset)   = 255;
				*(lpTemp + lOffset+1) = 255;
				*(lpTemp + lOffset+2) = 255;
				
			}
			//�����ǰ��Ϊ��ɫ�������ʱ�����ж�Ӧ������Ϊ��ɫ
			else
			{
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;
			}
		}
		//��ͼ�����ܵĵ�����Ϊ��ɫ
		for(i=0; i<height; i++)
		{
			lOffset = PixelOffset(i, 0, wBytesPerLine);
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;

		}
		
		for(i=0; i<height; i++)
		{
			lOffset = PixelOffset(i, width-1, wBytesPerLine);
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;

		}

	for (i=0; i<width; i++)
		{
			lOffset = PixelOffset(0, i, wBytesPerLine);
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;

		}
	
	for (i=0; i<width; i++)
		{
			lOffset = PixelOffset(height-1, i, wBytesPerLine);
				*(lpTemp + lOffset)   = 0;
				*(lpTemp + lOffset+1) = 0;
				*(lpTemp + lOffset+2) = 0;

		}
	//����ʱ����ĵ㿽����ԭ�����
	memcpy(lpData,lpTemp,wBytesPerLine*height);
	GlobalUnlock(hDIB);
	GlobalUnlock(hTempDIB);
	GlobalFree(hTempDIB);

}
void DIB::Dilation(HANDLE hDIB)
{
	LPBITMAPINFOHEADER lpbi;
	int height;
	int width;
	WORD wBytesPerLine;
	LPBYTE lpData;
	LPBYTE lpTemp;
	long lOffset;
	//�õ�ͼ��Ļ�����Ϣ
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width = lpbi->biWidth;
	wBytesPerLine = this->BytePerLine(hDIB);
	lpData = this->FindDIBBits(hDIB);
	//����һ�����������С��ͬ���ڴ�
	lpTemp = (LPBYTE) new BYTE[wBytesPerLine * height];
	
	
	long lOffsetJudge;
	for (int i=1; i<height-1; i++)
		for (int j=1; j<width-1; j++)
		{	
			lOffset = this->PixelOffset(i, j, wBytesPerLine);
			//�����ǰ��Ϊ��ɫ������ѭ��
			if(*(lpData + lOffset) == 255)
			{
				*(lpTemp + lOffset++) = 255;
				*(lpTemp + lOffset++) = 255;
				*(lpTemp + lOffset++) = 255;
				continue;
			}
			//���򿼲����������ĸ���
			else
			{	
				lOffsetJudge = this->PixelOffset(i-1, j, wBytesPerLine);
				//�������ĵ�Ϊ��ɫ
				if(*(lpData + lOffsetJudge) == 255)
				{	//����Ϊ��ɫ��������ѭ��
					*(lpTemp + lOffset++) = 255;
					*(lpTemp + lOffset++) = 255;
					*(lpTemp + lOffset++) = 255;
					continue;
				}

				//��������ĵ�
				lOffsetJudge = this->PixelOffset(i+1,j, wBytesPerLine);
				if(*(lpData + lOffsetJudge) == 255)
				{
					*(lpTemp + lOffset++) = 255;
					*(lpTemp + lOffset++) = 255;
					*(lpTemp + lOffset++) = 255;
					continue;
				}
				
				//������ߵĵ�
				lOffsetJudge = this->PixelOffset(i,j-1, wBytesPerLine);
				if(*(lpData + lOffsetJudge) == 255)
				{
					*(lpTemp + lOffset++) = 255;
					*(lpTemp + lOffset++) = 255;
					*(lpTemp + lOffset++) = 255;
					continue;
				}
				//�����ұߵĵ�
				lOffsetJudge = this->PixelOffset(i,j+1, wBytesPerLine);
				if(*(lpData + lOffsetJudge) == 255)
				{
					*(lpTemp + lOffset++) = 255;
					*(lpTemp + lOffset++) = 255;
					*(lpTemp + lOffset++) = 255;
					continue;
				}
				//����������Ҷ��Ǻ�ɫ�㣬�����ʱ����ĵ�����Ϊ��ɫ
				lOffset = this->PixelOffset(i,j,wBytesPerLine);
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;

			}
				
		}
		//����ͼ�����ܵĵ㣬����Ϊ��ɫ
		for(i=0; i<height; i++)
		{
			lOffset = this->PixelOffset(i, 0, wBytesPerLine);
			{
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;
			}
		}

		for(i=0; i<height; i++)
		{
			lOffset = this->PixelOffset(i, width-1, wBytesPerLine);
			{
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;
			}
		}

		for(i=0; i<width; i++)
		{
			lOffset = this->PixelOffset(0, i, wBytesPerLine);
			{
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;
			}
		}

		for(i=0; i<width; i++)
		{
			lOffset = this->PixelOffset(height-1, i, wBytesPerLine);
			{
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;
				*(lpTemp + lOffset++) = 0;
			}
		}
		//����ʱ����ĵ㿽����ԭ�����������
		memcpy(lpData, lpTemp, wBytesPerLine*height);
		delete [] lpTemp;
		GlobalUnlock(hDIB);

}

void DIB::DeleteFasleEye(HANDLE hDIB, CRect facelocation)
{
	LPBYTE lpData;
	LPBITMAPINFOHEADER lpbi;
	int height;
	int width;
	long lOffset;
	WORD wBytesPerLine;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width = lpbi->biWidth;
	lpData = this->FindDIBBits(hDIB);
	wBytesPerLine = this->BytePerLine(hDIB);
	for (int i=0; i<height; i++)
		for (int j=0; j<width; j++)
		{
			lOffset = this->PixelOffset(i, j, wBytesPerLine);
			if(*(lpData + lOffset) == 255)
			{
				if(i<(facelocation.bottom+facelocation.top)/2)
				{
					*(lpData + lOffset++) = 0;
					*(lpData + lOffset++) = 0;
					*(lpData + lOffset++) = 0;
				}
			}
		}
		GlobalUnlock(hDIB);
}

void DIB::DeleteScatePoint(HANDLE hDIB)
{	
	LPBITMAPINFOHEADER lpbi;
	int height;
	int width;
	LPBYTE lpData;
	WORD wBytesPerLine;
	long lOffset;
	
	//�õ�ͼ��Ļ�����Ϣ
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width  = lpbi->biWidth;
	wBytesPerLine = this->BytePerLine(hDIB);
	lpData = this->FindDIBBits(hDIB);
	
	for (int i=0; i<height; i++)
		for(int j=0; j<width; j++)
		{	
			//�õ�ƫ��
			lOffset = this->PixelOffset(i, j, wBytesPerLine);
			//�����ǰ��Ϊ��ɫ��
			if(*(lpData + lOffset) == 255)
			{	
				//�趨�ж�����
				for(int ii = 0;ii<ImgRange;ii++)
					for (int jj=0; jj<ImgRange; jj++)
						this->lab[ii][jj] = false;
					//�趨�жϳ���
					this->lenth=0;
					//�ж��Ƿ�Ϊ��ɢ��
				bool judge = this->IsScaterPoint(j, i, width,height,lpData,wBytesPerLine,3,this->lab);
				if(!judge)
				{	
					//����ɢ����Ѹõ�����Ϊ��ɫ
					*(lpData + lOffset++) = 0;
					*(lpData + lOffset++) = 0;
					*(lpData + lOffset++) = 0;
				}
			}
		}
	GlobalUnlock(hDIB);
}

void  DIB::  MouseMap(LPBYTE lpRgb, const LPBYTE lpYcc,  WORD wBytesPerLine, CRect faceLocation)
{	
	//�����ѭ����������������ʵ����͵�ƥ��
	for (int i=faceLocation.top; i<faceLocation.bottom; i++)
		for (int j=faceLocation.left; j<faceLocation.right; j++)
		{	
			//�õ�ƫ��
			long lOffset = this->PixelOffset(i, j, wBytesPerLine);
			//�õ�cr��cb����ֵ
			int cr = *(lpYcc+lOffset+1);
			int cb = *(lpYcc+lOffset+2);
			//��־
			bool lab;
			int mapm;
			//����cr����ֵ�趨��־
			cr = cr-143;
			if(cr <-5 || cr>5)
			{
				cr = 0;
				
			}
		
			cr *=cr;
			
			if(cr>16)
				 lab = true;
			else
				lab = false;
			//����cb��ʱֵ�趨��־
			cb= cb-120;
			if(cb<-5 || cb >5)
				
			{
				cb = 0;
				if(lab = true)
					lab = false;
			}
			//���cr��cb������ֵ�����趨�ķ�Χ֮�ڣ����趨��ɫλ��ɫ�������ɫ
			if(lab)
				mapm = 255;
			else
				mapm = 0;
			 
			*(lpRgb + lOffset++) = mapm;
			*(lpRgb + lOffset++) = mapm;
			*(lpRgb + lOffset++) = mapm;

			
		}
}

void DIB::MouthCenter(HANDLE hDIB, CRect faceLocation, CPoint &mouthLocation)
{
	LPBITMAPINFOHEADER lpbi;
	int height;
	int width;
	long lOffset;
	WORD wBytesPerLine;
	LPBYTE lpData;
	
	//�õ�ͼ��Ļ�����Ϣ
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width  = lpbi->biWidth;
	wBytesPerLine = this->BytePerLine(hDIB);
	lpData = this->FindDIBBits(hDIB);
	
	//������������������ۼ������������ص�x��y�����ص���
	int xnum = 0 ;
	int ynum = 0 ;
	int count = 0;
	for (int i=faceLocation.top; i<faceLocation.bottom; i++)
		for (int j=faceLocation.left; j<faceLocation.right; j++)
		{
			lOffset = this->PixelOffset(i, j, wBytesPerLine);
			//��ɫ��
			if(*(lpData + lOffset) == 255)
			{	
				//xֵ��
				xnum +=j;
				//yֵ��
				ynum +=i;
				//������
				count++;
			}
		}
		//�õ����ĵ�λ��
	mouthLocation.x = xnum/count;
	mouthLocation.y = ynum/count;
	
	//�����ĵ�����λ��ɫ
	lOffset = this->PixelOffset(mouthLocation.y, mouthLocation.x, wBytesPerLine);
	*(lpData + lOffset++) =0;
	*(lpData + lOffset++) =255;
	*(lpData + lOffset++) =0;

	GlobalUnlock(hDIB);
}

void DIB::EyeCenter(HANDLE hDIB, CRect faceLocation, CPoint &eye1, CPoint &eye2)
{
	LPBITMAPINFOHEADER lpbi;
	LPBYTE lpData;
	long lOffset;
	WORD wBytesPerLine;
	int height;
	int width;
	int pixelnum =0;
	int num =0;
	//�õ�ͼ�������Ϣ
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width  = lpbi->biWidth;
	lpData = this->FindDIBBits(hDIB);
	wBytesPerLine  = this->BytePerLine(hDIB);
	//������������
	for(int i=faceLocation.top; i<faceLocation.bottom; i++)
		for (int j=faceLocation.left; j<faceLocation.right; j++)
		{
			lOffset = this->PixelOffset(i, j, wBytesPerLine);
			//��ɫ��
			if(*(lpData + lOffset) == 255)
				//�ݹ�ͳ�����ز��޸�����ֵ
				this->RecursiveCal(lpData,i,j,wBytesPerLine,pixelnum,++num);
		}
		//��ʼ���۾�������
		eye1.x =0;
		eye1.y =0;
		eye2.x =0;
		eye2.y =0;
		//��ʼ�����ص����
		int eye1count=0;
		int eye2count =0;
		for (i=faceLocation.top; i<faceLocation.bottom; i++)
			for (int j=faceLocation.left; j<faceLocation.right; j++)
			{
				lOffset = this->PixelOffset(i, j, wBytesPerLine);
				//������ص����ֵΪ1
				if(*(lpData + lOffset) == 1)
				{	
					//�۾�1�ĺ��������������ϵ�ǰ�������ֵ
					eye1.x +=j;
					eye1.y +=i;
					eye1count++;
					//�ѵ�ǰ��ĳɰ�ɫ
					*(lpData + lOffset++) = 255;
					*(lpData + lOffset++) = 255;
					*(lpData + lOffset++) = 255;

				}
				//�����ǰ���ص���ֵΪ2
				else if(*(lpData + lOffset) == 2)
				{	
					//�۾�2�ĺ��������������ϵ�ǰ�������ֵ
					eye2.x +=j;
					eye2.y +=i;
					//���ص������һ
					eye2count++;
					//�ѵ�ǰ������Ϊ��ɫ
					*(lpData + lOffset++) = 255;
					*(lpData + lOffset++) = 255;
					*(lpData + lOffset++) = 255;
				}
			}
			//�����۾������ĵ�����
			eye1.x /=eye1count;
			eye1.y /=eye1count;
			eye2.x /=eye2count;
			eye2.y /=eye2count;
			//�����ĵ�����Ϊ��ɫ
			lOffset = this->PixelOffset(eye1.y, eye1.x ,wBytesPerLine);
			*(lpData + lOffset++) = 0;
			*(lpData + lOffset++) = 255;
			*(lpData + lOffset++) = 0;

			lOffset = this->PixelOffset(eye2.y, eye2.x ,wBytesPerLine);
			*(lpData + lOffset++) = 0;
			*(lpData + lOffset++) = 255;
			*(lpData + lOffset++) = 0;
	GlobalUnlock(hDIB);
}

void DIB::EllipseFace(HANDLE hDIB, CPoint mouth, CPoint eye1, CPoint eye2)
{
	LPBYTE lpData;
	LPBITMAPINFOHEADER lpbi;
	int width;
	int height;
	WORD wBytesPerLine;
	long lOffset;
	//�õ�ͼ��Ļ�����Ϣ
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
	height = lpbi->biHeight;
	width  = lpbi->biWidth;
	lpData = this->FindDIBBits(hDIB);
	wBytesPerLine = this->BytePerLine(hDIB);
	
	//��dda�㷨��������
	this->DdaLine(mouth,eye1,lpData,wBytesPerLine);
	this->DdaLine(mouth,eye2,lpData,wBytesPerLine);
	this->DdaLine(eye1,eye2,lpData,wBytesPerLine);
	
	//��Բ�����ĵ��������������
	int ellipsecenter_x;
	int ellipsecenter_y;
	int ellipseFocusTop_x;
	int ellipseFocusTop_y;
	int ellipseFocusBottom_x;
	int ellipseFocusBottom_y;
	
	//�����۾�����͵����������Բ�����ĵ�����
	ellipsecenter_x = (eye1.x + eye2.x + mouth.x )/3;
	ellipsecenter_y = (eye1.y + eye2.y)/2 -abs(eye2.x - eye1.x)/2;

	//����Ľ���
	ellipseFocusTop_x = ellipsecenter_x;
	ellipseFocusBottom_x = ellipsecenter_x;

	//����Ľ���
	ellipseFocusTop_y =  ellipsecenter_y + (eye1.y +eye2.y)/2 -mouth.y;
	ellipseFocusBottom_y = ellipsecenter_y - ((eye1.y +eye2.y)/2 -mouth.y)+2;

	//����
	int a = (eye1.x-eye2.x)*2-2;
	
	for (int i=0; i<height; i++)
		for (int j=0; j<width; j++)
		{	
			//�õ�һ���㵽��������ľ����
			int lenth = sqrt(pow(j-ellipseFocusTop_x,2)+pow(i-ellipseFocusTop_y,2))
				+sqrt(pow(j-ellipseFocusBottom_x,2)+ pow(i-ellipseFocusBottom_y,2));
			//�жϾ�����볤��Ĺ�ϵ
			if(lenth<2*a+2 && lenth >2*a-2)
			{	
				//�ѵ�����Ϊ��ɫ
				lOffset = this->PixelOffset(i, j, wBytesPerLine);
				*(lpData + lOffset++) = 0;
				*(lpData + lOffset++) = 255;
				*(lpData + lOffset++) = 0;
			}
		}


	GlobalUnlock(hDIB);
}


void DIB::DdaLine(CPoint from, CPoint end, LPBYTE lpData, WORD wBytesPerLine)
{	
	//x��y������
	float delta_x;
	float delta_y;
	//x��y������
	float x;
	float y;
	//x��y�ϵĲ�ֵ
	int dx;
	int dy;
	//�ܵĲ���
	int steps;
	int k;
	//�õ�x��y�Ĳ�ֵ
	dx = end.x - from.x;
	dy = end.y - from.y;
	//�ж�x��y�ϵĲ�ֵ��С��ȷ������
	if(abs(dx) > abs(dy))
	{
		steps = abs(dx);
	}
	else
	{
		steps = abs(dy);
	}

	//�õ�ÿ�������Ĵ�С
	delta_x = (float)dx / (float)steps;
	delta_y = (float)dy / (float)steps;
	//�趨x��y�����
	x = (float)from.x;
	y = (float)from.y;

	//�趨��ʼ�����ɫΪ��ɫ
	long lOffset = this->PixelOffset(y, x, wBytesPerLine);
	*(lpData + lOffset++) = 0;
	*(lpData + lOffset++) = 255;
	*(lpData + lOffset++) = 0;

	//���ݼ���õ��Ĳ�������ֱ���ϵĵ�������ɫ
	for (k=1;k<=steps; k++)
	{	
		//x��y�ֱ���ϸ��Ե�����
		x+=delta_x;
		y+=delta_y;
		//���õ����ɫ
		lOffset = this->PixelOffset(y, x, wBytesPerLine);
		*(lpData + lOffset++) = 0;
		*(lpData + lOffset++) = 255;
		*(lpData + lOffset++) = 0;

	}
	

}
