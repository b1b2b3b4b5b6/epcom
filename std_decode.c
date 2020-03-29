#include "std_decode.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_INFO
#define WORKSZ 128 * 1024

//Data that is passed from the decoder function to the infunc/outfunc functions.
typedef struct {
    const unsigned char *inData;	//Pointer to jpeg data
    int inPos;						//Current position in jpeg data
    uint16_t **outData;				//Array of IMAGE_H pointers to arrays of IMAGE_W 16-bit pixel values
    int outW;						//Width of the resulting file
    int outH;						//Height of the resulting file
} JpegDev;


//Input function for jpeg decoder. Just returns bytes from the inData field of the JpegDev structure.
static UINT infunc(JDEC *decoder, BYTE *buf, UINT len) 
{
    //Read bytes from input file
    JpegDev *jd=(JpegDev*)decoder->device;
    if (buf!=NULL) memcpy(buf, jd->inData+jd->inPos, len);
    jd->inPos+=len;
    return len;
}

//Output function. Re-encodes the RGB888 data from the decoder as big-endian RGB565 and
//stores it in the outData array of the JpegDev structure.
static UINT outfunc(JDEC *decoder, void *bitmap, JRECT *rect) 
{
    JpegDev *jd=(JpegDev*)decoder->device;
    uint8_t *in=(uint8_t*)bitmap;
    for (int y=rect->top; y<=rect->bottom; y++) {
        for (int x=rect->left; x<=rect->right; x++) {
            //We need to convert the 3 bytes in `in` to a rgb565 value.
            uint16_t v=0;
            v|=((in[0]>>3)<<11);
            v|=((in[1]>>2)<<5);
            v|=((in[2]>>3)<<0);
            //The LCD wants the 16-bit value in big-endian, so swap bytes
            //v=(v>>8)|(v<<8);
            jd->outData[y][x]=v;
            in+=3;
        }
    }
    return 1;
}

//Size of the work space for the jpeg decoder.


//Decode the embedded image into pixel lines that can be used with the rest of the logic.
esp_err_t std_decode_image(int w, int h, uint8_t *in, uint16_t *img) 
{
    char *work=NULL;
    int r;
    JDEC decoder;
    JpegDev jd;
    esp_err_t ret=ESP_OK;

    uint16_t **pixels;
    pixels = STD_CALLOC(sizeof(uint16_t *), h);
    for(int n = 0; n < h; n++)
    {
        pixels[n] = img + n * w;
    }

    //Allocate the work space for the jpeg decoder.
    work=calloc(WORKSZ, 1);
    if (work==NULL) {
        STD_LOGE("Cannot allocate workspace");
        ret=ESP_ERR_NO_MEM;
        goto err;
    }

    //Populate fields of the JpegDev struct.
    jd.inData= in;
    jd.inPos= 0;
    jd.outData= pixels;
    jd.outW= w;
    jd.outH= h;
    
    //Prepare and decode the jpeg.
    r=jd_prepare(&decoder, infunc, work, WORKSZ, (void*)&jd);
    if (r!=JDR_OK) {
        STD_LOGE("Image decoder: jd_prepare failed (%d)", r);
        ret=ESP_ERR_NOT_SUPPORTED;
        goto err;
    }
    r=jd_decomp(&decoder, outfunc, 0);
    if (r!=JDR_OK) {
        STD_LOGE("Image decoder: jd_decode failed (%d)", r);
        ret=ESP_ERR_NOT_SUPPORTED;
        goto err;
    }
    
    //All done! Free the work area (as we don't need it anymore) and return victoriously.
    STD_FREE(work);
    STD_FREE(pixels);
    return ret;
err:
    STD_FREE(pixels);
    return ret;
}
