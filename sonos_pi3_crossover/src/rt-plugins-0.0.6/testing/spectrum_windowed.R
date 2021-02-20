spectrum.windowed <- function( y, fs=44100, fftlen=2^16, plot=TRUE, max0=FALSE,... ) {

  # window the data:
  require(fftw);
  hann.window <- 0.5*(1 - cos(2*pi*(0:fftlen)/fftlen)); # (length is fftlen+1)
  hann.window <- hann.window[-(fftlen+1)]; # drop last sample to make it periodic
  coherent.gain <- 0.5;

  kmax <- floor(2*length(y)/fftlen); # do kmax fft's (with 50% overlap)
  startinds <- floor( seq(1, length(y)-fftlen, length=kmax ) );

  cat("averaging",kmax,"records of length",fftlen,"...\n");

  my.spec <- NULL;
  for (k in startinds) {

#    cat("\ncalculating spectrum at starting index", k, "...\n");
    y.wind <- y[k:(k+fftlen-1)] * hann.window;
    Y <- FFT(y.wind);
    M <- length(Y);

    # select only first half of fft, double it (fft is 2-sided)
    # and discard the DC component:
    Y <- 2 * Mod( Y[2:(length(Y)/2)] ) / length(y.wind);

    if (is.null(my.spec)) my.spec <- rep(0,length(Y));
    my.spec <- my.spec + Y;
#    cat("got peak", max(Y), "\n");
  }

  my.spec <- my.spec / kmax / coherent.gain;

  f <- ( 1:length(Y) ) * fs / M;

  if (max0) my.spec <- my.spec / max(my.spec);

  if (plot) {
    plot(f, 20*log10(my.spec), xlab="Frequency [Hz]", ylab="Level [dBFS]", type="l", ...);
  }

  return( list(f=f,Y=my.spec) );
}

