/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#ifndef _SIMPLE_CONSOLE_GCM_H_
#define _SIMPLE_CONSOLE_GCM_H_

#include "FWGCMApplication.h"

class DbgFontApp : public FWGCMApplication
{
public:
	DbgFontApp();
	~DbgFontApp();

	virtual bool	onInit(int argc, char **ppArgv);
	virtual void	onRender();
	virtual	void	onShutdown();
	virtual void	onSize(const FWDisplayInfo & dispInfo);
	virtual bool    onUpdate();

private:
	bool		dbgFontInit(void);
	void		dbgFontExit(void);
	void		dbgFontPut(void);
	void		dbgFontDraw(void);

	CellDbgFontConsoleId mDbgFontID[2];
	unsigned int	mFrame;

};

#endif //_SIMPLE_CONSOLE_GCM_H_
