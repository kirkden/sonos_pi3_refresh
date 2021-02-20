# generate_mod_sine.R
# Calculate and save a modulated sine wave signal.
generate_mod_sine <- function( f, T, fs=44100, bitdepth=16, level=1.0,
                               dither=TRUE, filename="modsine.wav" ) {
# f:      carrier frequency [Hz]
# T:      total time [s]
# fs:     sampling rate (Hz)
# level:  amplitude scaling (1.0 = 0 dB)
#
# A sine wave is generated, modulated by another sine at 1/10 its
# frequency.  To be used as a test signal for measuring harmonic and
# intermodulation distortion as per:
# http://www.linkwitzlab.com/mid_dist.htm
# The result is written to 'filename' in wav format.
#

  f2 <- 0.05*f;  # modulation frequency
  period <- 1/f2;
  T <- (T %/% period) * period;  # truncate T to an integer number of full cycles
  time <- seq(0,T-1/fs,by=1/fs);

  # Create the modulated sine tone
  w <- 2*pi*f;
  w2 <- 2*pi*f2;
  x <- level * ( sin( w*time ) * 0.5*(cos(w2*time)-1) );

  if (dither) {  # add dither before requantization
    LSB <- 2^(-bitdepth);
    n <- length(time);
    # high-passed triangular dither signal:
    dither.sig <- 0.5*LSB*(runif(n,min=-1,max=1) + runif(n,min=-1,max=1));
    dither.sig <- filter( dither.sig, c(1,-1), circular=TRUE );
    xout <- x + as.vector(dither.sig);
  } else {
    xout <- x;
  }

  # use 'tuneR' for i/o: package 'sound' adds distortion that can't be dithered!
  require(tuneR);
  s <- Wave( left=round(2^(bitdepth-1) * xout), samp.rate=fs, bit=bitdepth );
  writeWave( s, filename, extensible=TRUE );

  # we have to convert to wavpcm format or ecasound won't work:
  system( sprintf("sox %s tmp.wavpcm", filename) );
  system( sprintf("mv tmp.wavpcm %s", filename) );
  system( "rm -f tmp.wavpcm" );

}

