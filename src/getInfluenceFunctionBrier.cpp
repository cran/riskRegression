// [[Rcpp::depends(RcppArmadillo)]]
#include "arma-wrap.h"

using namespace Rcpp;
using namespace arma;

// Calculate influence function for competing risk case/survival case with Nelson-Aalen censoring.
// Author: Johan Sebastian Ohlendorff
// [[Rcpp::export(rng=false)]]
NumericVector getInfluenceFunctionBrierKMCensoring(double tau,
                                                   NumericVector time,
                                                   NumericVector risk,
                                                   NumericVector status,
                                                   NumericVector GTiminus,
                                                   double brier) {
  // check if any of the vectors have NAs and also that the vectors have the same lengths
  checkNAs(time, GET_VARIABLE_NAME(time));
  checkNAs(status, GET_VARIABLE_NAME(status));
  checkNAs(tau, GET_VARIABLE_NAME(tau));
  checkNAs(risk,GET_VARIABLE_NAME(risk));
  checkNAs(GTiminus,GET_VARIABLE_NAME(GTiminus));
  checkNAs(brier, GET_VARIABLE_NAME(brier));
  compareLengths(time,status);
  compareLengths(status,risk); 
  compareLengths(risk,GTiminus);
  
  // Thomas' code from IC of Nelson-Aalen estimator
  //initialize first time point t=0 with data of subject i=0
  int n = time.size();
  NumericVector ic(n);
  arma::uvec sindex(n,fill::zeros);
  arma::vec utime=unique(time);
  int nu=utime.size();
  arma::vec atrisk(nu);
  arma::vec Cens(nu,fill::zeros);
  arma::vec hazardC(nu,fill::zeros);
  arma::vec MC_term2(nu,fill::zeros);
  int t=0;
  double Y = (double) n;
  atrisk[0]=Y;
  Cens[0]=(1-(status[0] != 0));
  hazardC[0]=Cens[0]/Y;
  MC_term2[0]+=hazardC[0];
  //loop through time points until last subject i=(n-1)
  for (int i=1;i<=n;i++) {
    if (i<n && time[i]==time[i-1]){// these are tied values
      Cens[t] +=(1-(status[i] != 0));
      Y-=1;
      sindex[i]=t;    // index pointer from subject i to unique time point t
    }else{
      utime[t]=time[i-1];
      hazardC[t]=Cens[t]/atrisk[t];
      MC_term2[t]=hazardC[t]*n/atrisk[t];
      //initialize next time point with data of current subject i
      if (i<n){
        t++;
        sindex[i]=t;    // index pointer from subject i to unique time point t
        Y-=1;
        atrisk[t]=Y;
        Cens[t]=(1-(status[i] != 0));
      }
    }
  }
  MC_term2 = arma::cumsum(MC_term2);

  // find first index such that k such that tau[k] <= tau but tau[k+1] > tau
  auto lower = std::upper_bound(time.begin(), time.end(), tau);
  int firsthit = std::distance(time.begin(), lower) -1;
  double icpart1 = 0;
  double icpart2 = 0;
  for (int i = 0; i <= firsthit; i++){
    if (status[i] == 1){
      icpart2 += (1-2*risk[i])/GTiminus[i];
    }
  }
  int tieIter = 0;
  // can do while loops together
  while ((tieIter < n) && (time[tieIter] == time[0])) {
    if ((time[tieIter] <= tau) && (status[tieIter]==1)){
      icpart2 -= (1.0-2.0*risk[tieIter]) / GTiminus[tieIter];
    }
    tieIter++;
  }
  int upperTie = tieIter-1;
  double icterm{}, fihattau{},firstterm{};
  int j;
  if (firsthit==-1){
    for (int i = 0; i<n; i++){
      ic[i] = risk[i]*risk[i]-brier;
    }
  }
  else {
    for (int i = 0; i<n; i++){
      if (i > firsthit){
        j = firsthit;
      }
      else {
        j = i;
      }
      if (utime[sindex[j]] < time[i]){
        fihattau = - MC_term2[sindex[j]];
      }
      else {
        fihattau =  (1-(status[i] != 0))*n/atrisk[sindex[i]]- MC_term2[sindex[i]];
      }
      icterm = 1.0 / n * (icpart1+fihattau*icpart2);
      if (upperTie == i){
        int tieIter = i+1;
        while ((tieIter < n) && (time[tieIter] == time[i+1])) {
          if ((time[tieIter] <= tau) && (status[tieIter]==1)){
            icpart1 -= (1.0-2.0*risk[tieIter])*(MC_term2[sindex[i]]) / GTiminus[tieIter];
            icpart2 -= (1.0-2.0*risk[tieIter]) / GTiminus[tieIter];
          }
          tieIter++;
        }
        upperTie = tieIter-1;
      }
      if (time[i] <= tau && status[i] == 1){
        firstterm = (1.0-2.0*risk[i]) / GTiminus[i];
      }
      else {
        firstterm = 0.0;
      }
      ic[i] = firstterm + icterm + risk[i]*risk[i]-brier;
    }
  }
  return ic;
}

// [[Rcpp::export(rng=false)]]
NumericVector getInfluenceFunctionBrierKMCensoringUseSquared(double tau,
                                                             NumericVector time,
                                                             NumericVector residuals,
                                                             NumericVector status) {
  // Thomas' code from IC of Nelson-Aalen estimator
  //initialize first time point t=0 with data of subject i=0
  int n = time.size();
  NumericVector ic(n);
  arma::uvec sindex(n,fill::zeros);
  arma::vec utime=unique(time);
  int nu=utime.size();
  arma::vec atrisk(nu);
  arma::vec Cens(nu,fill::zeros);
  arma::vec hazardC(nu,fill::zeros);
  arma::vec MC_term2(nu,fill::zeros);
  int t=0;
  double Y = (double) n;
  atrisk[0]=Y;
  Cens[0]=(1-(status[0] != 0));
  hazardC[0]=Cens[0]/Y;
  MC_term2[0]+=hazardC[0];
  //loop through time points until last subject i=(n-1)
  for (int i=1;i<=n;i++) {
    if (i<n && time[i]==time[i-1]){// these are tied values
      Cens[t] +=(1-(status[i] != 0));
      Y-=1;
      sindex[i]=t;    // index pointer from subject i to unique time point t
    }else{
      utime[t]=time[i-1];
      hazardC[t]=Cens[t]/atrisk[t];
      MC_term2[t]=hazardC[t]*n/atrisk[t];
      //initialize next time point with data of current subject i
      if (i<n){
        t++;
        sindex[i]=t;    // index pointer from subject i to unique time point t
        Y-=1;
        atrisk[t]=Y;
        Cens[t]=(1-(status[i] != 0));
      }
    }
  }
  MC_term2 = arma::cumsum(MC_term2);
  
  // find first index such that k such that tau[k] <= tau but tau[k+1] > tau
  // find first index such that k such that tau[k] <= tau but tau[k+1] > tau
  auto lower = std::upper_bound(time.begin(), time.end(), tau);
  int firsthit = std::distance(time.begin(), lower) -1;
  double icpart1 = 0;
  double icpart2 = 0;
  double icpart = 0;
  for (int i = 0; i < n; i++){
    if (status[i] == 1 && time[i] <= tau){
      icpart2 += residuals[i];
    }
    else if (time[i] > tau){
      icpart += residuals[i];
    }
  }
  icpart = icpart / ( (double) n);
  double brier = mean(residuals);
  int tieIter = 0;
  // can do while loops together
  while ((tieIter < n) && (time[tieIter] == time[0])) {
    if ((time[tieIter] <= tau) && (status[tieIter]==1)){
      icpart2 -= residuals[tieIter];
    }
    tieIter++;
  }
  int upperTie = tieIter-1;
  double icterm{}, icterm2{}, fihattau{};
  int j;
  for (int i = 0; i<n; i++){
    if (i > firsthit){
      j = firsthit;
    }
    else {
      j = i;
    }
    if (j==-1){
      fihattau = 0.0;
    }
    else if (utime[sindex[j]] < time[i]){
      fihattau = - MC_term2[sindex[j]];
    }
    else {
      fihattau =  (1-(status[i] != 0))*n/atrisk[sindex[i]]- MC_term2[sindex[i]];
    }
    icterm = 1.0 / n * (icpart1+fihattau*icpart2);
    icterm2 = icpart * fihattau;
    if (upperTie == i){
      int tieIter = i+1;
      while ((tieIter < n) && (time[tieIter] == time[i+1])) {
        if ((time[tieIter] <= tau) && (status[tieIter]==1)){
          icpart1 -= residuals[tieIter]*MC_term2[sindex[i]];
          icpart2 -= residuals[tieIter];
        }
        tieIter++;
      }
      upperTie = tieIter-1;
    }
    ic[i] = residuals[i] - brier + icterm+icterm2;
  }
  return ic;
}
