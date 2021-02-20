dyn.load("./mls.so");

# generate Nth-order mls signal (vector 0's and 1's)
generatemls <- function(N)
  if (N <= 18) {
    return( .C( "GenerateMls",
            result=integer(2^N-1),
            as.integer(N) )$result );
  } else {
    stop("mls order of ",N," is too large (max is 18)\n");
  }

# fake the response of a system to an mls signal
generatetestsignal <- function(mlssig)
   .C( "GenerateSignal",
        as.integer(mlssig),
        result=double(length(mlssig)),
        as.integer(length(mlssig)) )$result

# recover impulse response given the system's response (sysresp) to
# an mls signal (mlssig) or order N (note that this means both mlssig
# and sysresp must have length 2^N-1)
recoverimpulseresponse <- function(mlssig,sysresp,N)
  if ((length(mlssig) == (mylen <- 2^N-1)) && (length(sysresp) == mylen)) {
    return( .C( "RecoverImpulseResp",
             as.integer(mlssig),
             as.double(sysresp),
             result=double(length(mlssig)+1),
             as.integer(N),
             as.integer(length(mlssig)) )$result );
    } else {
      stop("mlssig and sysresp have different lengths\n");  
    }

