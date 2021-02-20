# ladspa_testing.R
# RT 16.07.2015
# An R script to measure and plot actual responses (magnitude/phase)
# for all filters in the rt-plugins collection.  Measurements are
# by mls techniques.

rm(list=ls());
graphics.off();

require(sound);

source("./mls.R");  # load mls functions

N <- 17;         # mls order
dt <- 1/44100;   # sampling period
amp <- 0.25;     # mls signal amplitude
reps <- 3;       # reps of mls signal
savemls <- TRUE;

plotphase <- FALSE;

respfn <- "mls_resp.wav";

mlsfn <- paste("mls_",N,"_x",reps,".wav",sep="");

# generate mls signal ====================================================
cat("\ngenerating mls signal of order",N,"...\n");

mls.sig <- generatemls( N );                 # construct mls signal
m <- length( mls.sig );

cat( "generated", m, " samples (", m*dt, "sec )...\n" );

if (savemls) {
  y <- amp * (-2 * mls.sig + 1);   # shift [0,1] to [-1,1] and scale amplitude
  yL <- rep( y, reps );      # left channel

  #yR <- rep( 0, m * reps );  # right channel
  #s <- as.Sample( rbind( yL, yR ), rate=44100, bits=16 );  # save stereo wav
  s <- as.Sample( yL, rate=44100, bits=16 );  # save mono wav

  cat( "saving", reps, "reps of mls signal to", mlsfn, "...\n" );
  saveSample( s, mlsfn, overwrite=TRUE );
}

# function to measure and plot magnitude/phase response of a given filter:
makegraph <- function(filter,reffreq,reflevel,xlim=c(20,20000),ylim=c(-20,5)) {

  # apply filter to mls signal and record output to respfn ===================
	cmd <- sprintf("ecasound -x -q -i %s -el:%s -f:16,1,44100 -o %s", mlsfn, filter, respfn);
	cat("\nrunning filter:\n  ",cmd,"\n");
	system(cmd);

	# deconvolve impulse response ==============================================
	cat("loading", respfn, "...\n");
	s1 <- loadSample( respfn );      # load recorded response
	y1 <- s1$sound[1,];              # keep only channel 1 if stereo

	# extract response to 3rd of 3 mls reps to get steady state:
	cat("extracting middle of 3 mls responses...\n");
	inds <- seq(to=length(y1),length=m);
	resp1.sig <- y1[inds];

	cat("deconvolving impulse response...\n");
	impulse.resp1 <- recoverimpulseresponse( mls.sig, resp1.sig, N ) / amp;

	# get magnitude and phase responses ========================================
	cat("extracting magnitude and phase...\n");
	Y1 <- fft(impulse.resp1);
	M <- length(Y1);

	# standard fft stuff:
	Y1 <- Y1[2:(length(Y1)/2)];	          # select 1st half of fft; discard DC component
	phi <- Arg(Y1); 	                    # phase response
	Y1 <- Mod( Y1 );                    	# magnitude response
	f <- ( 1:length(Y1) ) / ( M * dt );  	# fft frequencies

  # plot magnitude response ========================================================
	x11(width=6,height=4);
	plot( f, 20*log10(Y1), type="l", log="x", col="blue", lwd=2, xlim=xlim, ylim=ylim, yaxt="n",
		    xlab="frequency [Hz]", ylab="level [dB]", main=sprintf("Magnitude Response\n %s",filter) );
  axis(2, at=3*(-6:1));
  axis(4, at=3*(-6:1));

	abline( v=reffreq, col="grey" );
	abline( h=c(0,reflevel), col="grey" );

  # plot phase response ========================================================
	if (plotphase) {
		x11(width=6,height=4);
		plot( f, phi*180/pi, type="l", log="x", col="blue", lwd=2, xlim=xlim, ylim=c(-180,180), yaxt="n",
				  xlab="frequency [Hz]", ylab="phase [degrees]", main=sprintf("Phase Response\n %s",filter) );
		axis(2, at=c(-180,-90,0,90,180) );
		axis(4, at=c(-180,-90,0,90,180) );
		abline( v=reffreq, col="grey" );
		abline( h=0, col="grey" );
  }

} # end of makeplot

# make plots for various filters...
makegraph( "RTparaeq,-12,500,1.5", 500, -12 );
makegraph( "RTlowshelf,5,500,0.7", 500, 5, ylim=c(-5,5) );
makegraph( "RThighshelf,3,500,0.7", 500, 3, ylim=c(-3,3) );
makegraph( "RTlowpass1,1000", 1000, -3, xlim=c(100,15000) );
makegraph( "RThighpass1,1000", 1000, -3 );
makegraph( "RTlowpass,500,0.7", 500, -3, xlim=c(20,7000) );
makegraph( "RThighpass,500,0.7", 500, -3 );
makegraph( "RTlr4lowpass,500", 500, -6, xlim=c(20,3000) );
makegraph( "RTlr4hipass,500", 500, -6, xlim=c(100,20000) );
makegraph( "RTallpass1,500", 500, 0, ylim=c(-3,3) );
makegraph( "RTallpass2,500,0.7", 500, 0, ylim=c(-3,3) );

