﻿//---------------------------------------------------------------------------
#include    "main.h"
#include    "app.h"
//---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	if(app_run() == false)
	{
		return -1;
	}
	return 0;
}
//---------------------------------------------------------------------------