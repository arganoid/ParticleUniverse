#pragma warning(disable: 4786)

#include <cstdio>
#include <cmath>
#include <ctime>

#include <sys\stat.h>

#include "ParticleUniverseGame.h"


int main(void)
{
	/* Seed the random number generator with current time. */
	srand((unsigned)time(NULL));

	ParticleUniverseGame* g = new ParticleUniverseGame();

	g->MainLoop();

	exit(0);
}
