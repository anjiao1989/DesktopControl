#pragma once

size_t __inline Base64_TargetLen(size_t cbSrc)
{
	return ((cbSrc / 3) + 1) * 4;
}
size_t __inline Base64_SrcLen(size_t cbTarget)
{
	return (cbTarget / 4) * 3;
}

size_t Base64_Decode(char *pDest, const char *pSrc, size_t srclen);
size_t Base64_Encode(char *pDest, const char *pSrc, size_t srclen);
