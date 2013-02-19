//**********************************************
//decoder.h
//Unipipy @2011
//Main C++ entrance of this decoder
//**********************************************
#ifndef INTERFACE_DECODER_H
#define INTERFACE_DECODER_H

#include <inttypes.h>

struct LentCodecContext;
struct LentFrame;

class DecodeCore
{
public:

	DecodeCore();
	~DecodeCore();

	void Set_Thread(int n);
	void Clean();
	void FlushDecoder();


	void DecodeFrame(uint8_t *InputNalBuffer, uint8_t **OutputYUVBuffer, long *pDataLength, int64_t* pts, int *width, int stride[3]);

	int StartDecoder(int compatibility=0x7FFFFFFF);
	void UninitDecoder();
	bool IsReleased();

private:

	int i_thread;

	LentCodecContext *ctx;
	LentFrame *frame;
};

#endif