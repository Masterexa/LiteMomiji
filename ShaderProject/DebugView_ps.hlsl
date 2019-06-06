#include "Test.hlsli"


PSOut main(VSOut psin)
{
	PSOut psout = (PSOut)0;

	psout.color.rgb = normalize(psin.normal);
	psout.color.a	= psin.color.a;


	return psout;
}