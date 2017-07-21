#include "stdafx.h"

#include "Utility.h"

void GetFileNameWithoutExtension(LPTSTR path, LPTSTR filename, const int length)
{
	const int sourceLength = _tcslen(path);

	if (sourceLength <= 0) return;

	TCHAR* i = path + sourceLength;
	while (*i != _T('.') && i > path)
	{
		i--;
	}

	TCHAR* j = i;
	while (true)
	{
		if (j <= path + 1 || *(j - 1) == _T('\\')) break;
		j--;
	}

	int len = i - j;

	if (len + 1 < length)
	{
		_tcsnccpy_s(filename, length, j, len);
	}

	filename[len + 1] = '\0';
}
