#ifndef __XPT_CLIENT_H__
#define __XPT_CLIENT_H__

#include"global.h"

struct Xpt_Head
{
	uint32 head;

	convert()
	{
		head = ntohl_ex(head);
	}

};

#endif //__XPT_CLIENT_H__