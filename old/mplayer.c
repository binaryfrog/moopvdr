#include <stdlib.h>
#include <unistd.h>

void run_mplayer(const char *wd, const char *file)
{
	if(fork() > 0)
	{
		chdir(wd);
		execlp("mplayer", "mplayer",
				"-vo", "xvmc:deint-one",
				"-vc", "ffmpeg12mc",
				"-aspect", "4:3",
				"-monitoraspect", "4:3",
				"-aid", "0",
				"-fs",
				file,
				NULL);
		exit(0);
	}
}

