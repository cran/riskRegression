// [[Rcpp::depends(RcppArmadillo)]]
#include "arma-wrap.h"

using namespace Rcpp;
using namespace arma;

// Calculate influence function for competing risk case/survival case with Nelson-Aalen censoring.
// Equation lines correspond to the document influenceFunctionAUC.pdf in riskRegressionStudy
// Author: Johan Sebastian Ohlendorff
// [[Rcpp::export(rng = false)]]
NumericVector getInfluenceFunctionAUCKMCensoring(NumericVector time,
                                                 NumericVector status,
                                                 double tau,
                                                 NumericVector risk,
                                                 NumericVector GTiminus,
                                                 double Gtau,
                                                 double auc,
                                                 bool tiedValues) {
  // check if any of the vectors have NAs and also that the vectors have the same lengths
  checkNAs(time, GET_VARIABLE_NAME(time));
  checkNAs(status, GET_VARIABLE_NAME(status));
  checkNAs(tau, GET_VARIABLE_NAME(tau));
  checkNAs(risk,GET_VARIABLE_NAME(risk));
  checkNAs(GTiminus,GET_VARIABLE_NAME(GTiminus));
  checkNAs(Gtau,GET_VARIABLE_NAME(Gtau));
  checkNAs(auc, GET_VARIABLE_NAME(auc));
  compareLengths(time,status);
  compareLengths(status,risk); 
  compareLengths(risk,GTiminus);
  
  // Thomas' code from IC of Nelson-Aalen estimator, i.e. calculate the influence function of the hazard
  // initialize first time point t=0 with data of subject i=0
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
  if (firsthit == -1){
    firsthit = 0;
  }

  // P(tilde{T_i} > tau)
  double Probmu = double ((n-(firsthit+1))) / n;
  // If ties = TRUE, there are equalities for X in the below
  //P(X < X_i, tilde{T_i} > tau)
  NumericVector Probnu(n);
  // int 1*(X_i < x, t <= tau) / G(t-) dP(t,1,x) 
  NumericVector eq1112part(n);
  // int 1*(X_i > x, t <= tau) / G(t-) dP(t,2,x) 
  NumericVector eq1314part(n);
  // risk ordering, i.e. order according to risk 
  IntegerVector ordering(n);
  std::iota(ordering.begin(), ordering.end(), 0);
  std::sort(ordering.begin(), ordering.end(),
            [&](int x, int y) { return risk[x] < risk[y]; });
  // efficient computation of eq1112part and eq1314part by using the ordering of risk
  // the integrals differ if we are trying to compute the part concerning ties in risk
  if (!tiedValues){
    int jCurr = 0, jPrev = 0, i = 0;
    double valCurr = 0, valPrev = 0;
    while (i < n){
      int tieIter = i;
      while (tieIter < n && risk[ordering[tieIter]]==risk[ordering[i]]){
        if (time[ordering[tieIter]] > tau){
          jCurr++;
        }
        else if (time[ordering[tieIter]] <= tau && status[ordering[tieIter]] == 2){
          valCurr += 1.0/(GTiminus[ordering[tieIter]]);
        }
        tieIter++;
      }
      for (int l = i; l < tieIter;l++){
        Probnu[ordering[l]] = ((double) jPrev)/n;
        eq1314part[ordering[l]] = valPrev;
      }
      i = tieIter;
      jPrev = jCurr;
      valPrev = valCurr;
    }
    valCurr = valPrev = 0;
    i = n-1;
    while (i >= 0){
      int tieIter = i;
      while (tieIter >= 0 && risk[ordering[tieIter]]==risk[ordering[i]]){
        if (time[ordering[tieIter]] <= tau && status[ordering[tieIter]] == 1){
          valCurr += 1.0/(GTiminus[ordering[tieIter]]);
        }
        tieIter--;
      } 
      for (int l = i; l > tieIter;l--){
        eq1112part[ordering[l]] = valPrev;
      }
      i = tieIter;
      valPrev = valCurr;
    }
  }
  else {
    int i = 0;
    while (i < n){
      int jProbnu = 0;
      double jPart1112 = 0, jPart1314 = 0;
      int tieIter = i;
      while (tieIter < n && risk[ordering[tieIter]]==risk[ordering[i]]){
        if (time[ordering[tieIter]] > tau){
          jProbnu++;
        }
        else if (time[ordering[tieIter]] <= tau && status[ordering[tieIter]] == 2){
          jPart1314+=1.0/(GTiminus[ordering[tieIter]]);
        }
        else if (time[ordering[tieIter]] <= tau && status[ordering[tieIter]] == 1){
          jPart1112+=1.0/(GTiminus[ordering[tieIter]]);
        }
        tieIter++;
      }
      for (int l = i; l < tieIter;l++){
        int ownWeightProbnu = time[ordering[l]] > tau ? 1 : 0;
        Probnu[ordering[l]] = ((double) jProbnu-ownWeightProbnu)/n;
        double ownWeight1314 = time[ordering[l]] <= tau && status[ordering[l]] == 2 ? 1.0/(((double) n)*GTiminus[ordering[l]]) : 0;
        eq1314part[ordering[l]] = jPart1314 - ownWeight1314;
        double ownWeight1112 = time[ordering[l]] <= tau && status[ordering[l]] == 1 ? 1.0/(((double) n)*GTiminus[ordering[l]]) : 0;
        eq1112part[ordering[l]] = jPart1112 - ownWeight1112;
      }
      i = tieIter;
    }
  }
  // Rcout << "Probnu: " << Probnu << "\n\n";
  // Rcout << "eq1314part: " << eq1314part << "\n\n";
  // Rcout << "eq1112part: " << eq1112part << "\n\n\n";
  
  // F_1(tau) = int 1_{t \leq tau} 1/ hat{G(t-)} dP(t,1) = Q(T_i <= tau, Delta_i = 1)
  // F_2(tau) = int 1_{t \leq tau} 1/ hat{G(t-)} dP(t,2) = Q(T_i <= tau, Delta_i = 2)
  double F1tau = 0, F2tau = 0, eq9term = 0;
  for (int i = 0; i <= firsthit; i++){
    if (status[i] == 1){
      F1tau += 1.0/GTiminus[i];
      eq9term += Probnu[i]/GTiminus[i];
    }
    else if (status[i] == 2){
      F2tau += 1.0/GTiminus[i];
    }
  }
  eq9term = eq9term / n;
  F1tau = F1tau / ((double) n);
  F2tau = F2tau / ((double) n);
  
  double eq16term = Probmu * F1tau;
  double mu1 = F1tau * Probmu / Gtau + F1tau*F2tau;
  double nu1 = auc * mu1;
  double eq10part1{}, eq12part1{},eq14part1{},eq17part1{},eq19part1{};
  double eq10part2 = n*eq9term;
  double eq12part2 = (nu1-1.0/(Gtau) * eq9term)*n*n;
  double eq14part2 = eq12part2;
  double eq17part2 = n*F1tau;
  double eq19part2 = n*F2tau;
  int tieIter = 0;
  // can do while loops together
  while ((tieIter < n) && (time[tieIter] == time[0])) {
    if ((time[tieIter] <= tau) && (status[tieIter]==1)){
      eq10part2 -= Probnu[tieIter] / GTiminus[tieIter];
      eq14part2 -= eq1314part[tieIter] / GTiminus[tieIter];
      eq17part2 -= 1.0 / GTiminus[tieIter];
    }
    else if  ((time[tieIter] <= tau) && (status[tieIter]==2)){
      eq12part2 -= eq1112part[tieIter] / GTiminus[tieIter];
      eq19part2 -= 1.0 / GTiminus[tieIter];
    }
    tieIter++;
  }

  int upperTie = tieIter-1;
  double eq8{}, eq9{}, eq10{}, eq11{},eq12{},eq13{},eq14{}, eq15{}, eq16{},eq17{},eq18{},eq19{},eq20{},eq21{}, fihattau{},eq1721part{};
  for (int i = 0; i<n; i++){
    int const j = i > firsthit ? firsthit : i;
    if (utime[sindex[j]] < time[i]){
      fihattau = - MC_term2[sindex[j]];
    }
    else {
      fihattau =  (1-(status[i] != 0))*n/atrisk[sindex[i]]- MC_term2[sindex[i]];
    }
    eq10 = 1.0 / (Gtau*n) * (eq10part1+fihattau*eq10part2);
    eq12 = 1.0 / (n*n) * (eq12part1+(fihattau-1)*eq12part2);
    eq14 = 1.0 / (n*n) * (eq14part1+(fihattau-1)*eq14part2);
    eq1721part = 1.0 / n * (eq17part1+fihattau*eq17part2);
    eq17 = Probmu / Gtau * eq1721part;
    eq19 = F1tau * (1.0 / n * (eq19part1+fihattau*eq19part2) - F2tau);
    eq21 = F2tau * (eq1721part - F1tau);
    
    // fast calculation of eq10 and eq17
    if (upperTie == i){
      int tieIter = i+1;
      while ((tieIter < n) && (time[tieIter] == time[i+1])) {
        if ((time[tieIter] <= tau) && (status[tieIter]==1)){
          eq10part1 -= Probnu[tieIter] * (MC_term2[sindex[i]]) / GTiminus[tieIter];
          eq14part1 -= eq1314part[tieIter] * (MC_term2[sindex[i]]+1.0) / GTiminus[tieIter];
          eq17part1 -= (MC_term2[sindex[i]]) / GTiminus[tieIter];
          eq10part2 -= Probnu[tieIter] / GTiminus[tieIter];
          eq14part2 -= eq1314part[tieIter] / GTiminus[tieIter];
          eq17part2 -= 1.0 / GTiminus[tieIter];
        }
        else if ((time[tieIter] <= tau) && (status[tieIter]==2)){
          eq12part1 -= eq1112part[tieIter] * (MC_term2[sindex[i]]+1.0) / GTiminus[tieIter];
          eq19part1 -= (MC_term2[sindex[i]]) / GTiminus[tieIter];
          eq12part2 -= eq1112part[tieIter] / GTiminus[tieIter];
          eq19part2 -= 1.0 / GTiminus[tieIter];
        }
        tieIter++;
      }
      upperTie = tieIter-1;
    }
    if ((time[i] <= tau) && (status[i] == 1)){
      eq8 = 1.0/(GTiminus[i]*Gtau)*Probnu[i];
      eq15 = 1.0/(GTiminus[i]*Gtau)*Probmu;
      eq13 = 1.0/ (n*GTiminus[i]) * eq1314part[i];
      eq20 = 1.0/GTiminus[i] * F2tau;
    }
    else {
      eq8 = eq13 = eq15 = eq20 = 0;
    }
    eq9 = (fihattau - 2.0)/Gtau * eq9term;
    eq16 = (fihattau - 2.0)/Gtau * eq16term;
    
    if ((time[i] <= tau) & (status[i] == 2)){
      eq11 = 1.0 / (n*GTiminus[i]) * eq1112part[i];
      eq18 = 1.0 / (GTiminus[i]) * F1tau;
    }
    else if (time[i] > tau){
      eq11 = 1.0 / (n*Gtau) * eq1112part[i];
      eq18 = 1.0 / (Gtau) * F1tau;
    }
    else {
      eq11 = eq18 = 0;
    }
    double IFnu = eq8+eq9+eq10+eq11+eq12+eq13+eq14;
    double IFmu = eq15+eq16+eq17+eq18+eq19+eq20+eq21;

    ic[i] = (IFnu * mu1 - IFmu * nu1)/(mu1*mu1);
  }
  return ic;
}

// [[Rcpp::export(rng = false)]]
NumericVector getInfluenceFunctionAUCKMCensoringCVPart(NumericVector time,
                                                       NumericVector status,
                                                       double tau,
                                                       NumericVector GTiminus,
                                                       double Gtau,
                                                       NumericMatrix thetahat,
                                                       IntegerVector whichControls,
                                                       IntegerVector whichCases,
                                                       double nu1tauPm) {
  // check for NAs and equal lengths of vectors
  checkNAs(time, GET_VARIABLE_NAME(time));
  checkNAs(status, GET_VARIABLE_NAME(status));
  checkNAs(tau, GET_VARIABLE_NAME(tau));
  checkNAs(GTiminus, GET_VARIABLE_NAME(GTiminus));
  checkNAs(Gtau, GET_VARIABLE_NAME(Gtau));
  // should also check matrix for NAs and the IntegerVectors
  compareLengths(time,status);
  compareLengths(status,GTiminus); 

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
  if (firsthit == -1){
    firsthit = 0;
  }
  
  // P(tilde{T_i} > tau)
  double Probmu = double ((n-(firsthit+1))) / n;
  NumericVector int1(n); // this is int \Theta(X_i, x) 1*(t > tau) / G(tau) dP(t,x) 
  NumericVector int2(n);  // int \Theta(x,X_i ) 1*(t <= tau) / G(t-) dP(t,1,x) 
  NumericVector int3(n);  // int \Theta(X_i,x) 1*(t <= tau) / G(t-) dP(t,2,x) 
  int noControlsI = 0;
  int noCasesI = 0;
  bool isCaseI = false; 
  for (int i = 0; i < n; i++){
    // keep track of indexing
    int iValue = 0;
    if (time[i] > tau){
      iValue = noControlsI;
      noControlsI++;
    }
    else if ((time[i] <= tau) && (status[i] == 1)){
      iValue = noCasesI;
      noCasesI++;
      isCaseI = true;
    }
    else if ((time[i] <= tau) && (status[i] == 2)){
      iValue = noControlsI;
      noControlsI++;
    } 
    if (isCaseI){
      int noControlsJ = 0;
      for (int j = 0; j < n; j++){
        if (time[j] > tau){
          int1[i] += thetahat(iValue,noControlsJ) / Gtau;
        }
        else if ((time[j] <= tau) && (status[j] == 2)){
          int3[i] += thetahat(iValue,noControlsJ) / GTiminus[j] ;
        } 
        noControlsJ++;
      }
    }
    else {
      int noCasesJ = 0;
      for (int j = 0; j < n; j++){
        if ((time[j] <= tau) && (status[j] == 1)){
          int2[i] += (1-thetahat(noCasesJ,iValue)) / GTiminus[j];
        }
        noCasesJ++;
      }
    }
    int1[i]=int1[i]/((double) n); 
  }
  // F_1(tau) = int 1_{t \leq tau} 1/ hat{G(t-)} dP(t,1) = Q(T_i <= tau, Delta_i = 1)
  // F_2(tau) = int 1_{t \leq tau} 1/ hat{G(t-)} dP(t,2) = Q(T_i <= tau, Delta_i = 2)
  double F1tau = 0, F2tau = 0, eq9term = 0;
  for (int i = 0; i <= firsthit; i++){
    if (status[i] == 1){
      F1tau += 1.0/GTiminus[i];
      eq9term += int1[i]/GTiminus[i];
    }
    else if (status[i] == 2){
      F2tau += 1.0/GTiminus[i];
    }
  }
  eq9term = eq9term / n;
  F1tau = F1tau / ((double) n);
  F2tau = F2tau / ((double) n);
  
  double eq16term = Probmu * F1tau;
  double mu1 = F1tau * Probmu / Gtau + F1tau*F2tau;
  double nu1 = nu1tauPm;
  double eq10part1{}, eq12part1{},eq14part1{},eq17part1{},eq19part1{};
  double eq10part2 = n*eq9term;
  double eq12part2 = (nu1-1.0/(Gtau) * eq9term)*n*n;
  double eq14part2 = eq12part2;
  double eq17part2 = n*F1tau;
  double eq19part2 = n*F2tau;
  int tieIter = 0;
  // can do while loops together
  while ((tieIter < n) && (time[tieIter] == time[0])) {
    if ((time[tieIter] <= tau) && (status[tieIter]==1)){
      eq10part2 -= int1[tieIter] / GTiminus[tieIter];
      eq14part2 -= int3[tieIter] / GTiminus[tieIter];
      eq17part2 -= 1.0 / GTiminus[tieIter];
    }
    else if  ((time[tieIter] <= tau) && (status[tieIter]==2)){
      eq12part2 -= int2[tieIter] / GTiminus[tieIter];
      eq19part2 -= 1.0 / GTiminus[tieIter];
    }
    tieIter++;
  }
  
  int upperTie = tieIter-1;
  double eq8{}, eq9{}, eq10{}, eq11{},eq12{},eq13{},eq14{}, eq15{}, eq16{},eq17{},eq18{},eq19{},eq20{},eq21{}, fihattau{},eq1721part{};
  for (int i = 0; i<n; i++){
    int const j = i > firsthit ? firsthit : i;
    if (utime[sindex[j]] < time[i]){
      fihattau = - MC_term2[sindex[j]];
    }
    else {
      fihattau =  (1-(status[i] != 0))*n/atrisk[sindex[i]]- MC_term2[sindex[i]];
    }
    eq10 = 1.0 / (Gtau*n) * (eq10part1+fihattau*eq10part2);
    eq12 = 1.0 / (n*n) * (eq12part1+(fihattau-1)*eq12part2);
    eq14 = 1.0 / (n*n) * (eq14part1+(fihattau-1)*eq14part2);
    eq1721part = 1.0 / n * (eq17part1+fihattau*eq17part2);
    eq17 = Probmu / Gtau * eq1721part;
    eq19 = F1tau * (1.0 / n * (eq19part1+fihattau*eq19part2) - F2tau);
    eq21 = F2tau * (eq1721part - F1tau);
    
    // fast calculation of eq10 and eq17
    if (upperTie == i){
      int tieIter = i+1;
      while ((tieIter < n) && (time[tieIter] == time[i+1])) {
        if ((time[tieIter] <= tau) && (status[tieIter]==1)){
          eq10part1 -= int1[tieIter] * (MC_term2[sindex[i]]) / GTiminus[tieIter];
          eq14part1 -= int3[tieIter] * (MC_term2[sindex[i]]+1.0) / GTiminus[tieIter];
          eq17part1 -= (MC_term2[sindex[i]]) / GTiminus[tieIter];
          eq10part2 -= int1[tieIter] / GTiminus[tieIter];
          eq14part2 -= int3[tieIter] / GTiminus[tieIter];
          eq17part2 -= 1.0 / GTiminus[tieIter];
        }
        else if ((time[tieIter] <= tau) && (status[tieIter]==2)){
          eq12part1 -= int2[tieIter] * (MC_term2[sindex[i]]+1.0) / GTiminus[tieIter];
          eq19part1 -= (MC_term2[sindex[i]]) / GTiminus[tieIter];
          eq12part2 -= int2[tieIter] / GTiminus[tieIter];
          eq19part2 -= 1.0 / GTiminus[tieIter];
        }
        tieIter++;
      }
      upperTie = tieIter-1;
    }
    if ((time[i] <= tau) && (status[i] == 1)){
      eq8 = 1.0/(GTiminus[i]*Gtau)*int1[i];
      eq15 = 1.0/(GTiminus[i]*Gtau)*Probmu;
      eq13 = 1.0/ (n*GTiminus[i]) * int3[i];
      eq20 = 1.0/GTiminus[i] * F2tau;
    }
    else {
      eq8 = eq13 = eq15 = eq20 = 0;
    }
    eq9 = (fihattau - 2.0)/Gtau * eq9term;
    eq16 = (fihattau - 2.0)/Gtau * eq16term;
    
    if ((time[i] <= tau) & (status[i] == 2)){
      eq11 = 1.0 / (n*GTiminus[i]) * int2[i];
      eq18 = 1.0 / (GTiminus[i]) * F1tau;
    }
    else if (time[i] > tau){
      eq11 = 1.0 / (n*Gtau) * int2[i];
      eq18 = 1.0 / (Gtau) * F1tau;
    }
    else {
      eq11 = eq18 = 0;
    }
    double IFnu = eq8+eq9+eq10+eq11+eq12+eq13+eq14;
    double IFmu = eq15+eq16+eq17+eq18+eq19+eq20+eq21;
    
    ic[i] = (IFnu * mu1 - IFmu * nu1)/(mu1*mu1);
  }
  return ic;
}

// [[Rcpp::export(rng = false)]]
NumericVector getInfluenceFunctionAUCBinaryCVPart(NumericVector Y,
                                                  NumericMatrix thetahat,
                                                  IntegerVector whichControls,
                                                  IntegerVector whichCases,
                                                  double nu1tauPm) {
  
  // check for NAs and equal lengths of vectors
  checkNAs(Y, GET_VARIABLE_NAME(Y));
  checkNAs(nu1tauPm, GET_VARIABLE_NAME(nu1tauPm));

  // Thomas' code from IC of Nelson-Aalen estimator
  //initialize first time point t=0 with data of subject i=0
  int n = Y.size();
  NumericVector ic(n);
  // Q(Y = 1)
  double q1 = ((double) whichCases.size())/((double ) n );
  // Q(Y=0) = 1- Q(Y=1)
  double q0 = 1-q1;
  double mu1 = q0 * q1;
  NumericVector int1(n); // this is int \Theta(X_i, x) 1*(y=0) dP(y,x) 
  NumericVector int2(n);  // int \Theta(x,X_i ) 1*(y=1) dP(y,x) 
  int noControlsI = 0;
  int noCasesI = 0;
  bool caseI = false;
  for (int i = 0; i < n; i++){
    // keep track of indexing
    int iValue = 0;
    if (Y[i]==0){
      iValue = noControlsI;
      noControlsI++;
    }
    else {
      iValue = noCasesI;
      noCasesI++;
      caseI = true;
    }
    int noControlsJ = 0;
    int noCasesJ = 0;
    int jValue = 0;
    if (caseI){
      for (int j = 0; j < n; j++){
        if (Y[j] == 0){
          jValue = noControlsJ;// unsure ...
          int1[i] += thetahat(iValue,jValue);
          noControlsJ++;
        }
      }
    }
    else {
      for (int j = 0; j < n; j++){
        if (Y[j] == 1){
          jValue = noCasesJ;
          int2[i] += 1-thetahat(jValue,iValue);
          noCasesJ++;
        }
      }
    }
    int1[i]=int1[i]/((double) n); 
    int2[i]=int2[i]/((double) n); 
  }

  for (int i = 0; i<n; i++){
    double IFnu = 0;
    double IFmu = 0;
    if (Y[i]==0){
      IFnu = int2[i];
      IFmu = q0;
    }
    else {
      IFnu = int1[i];
      IFmu = q1;
    }
    
    ic[i] = (IFnu * mu1 - IFmu * nu1tauPm)/(mu1*mu1);
  }
  return ic;
}


