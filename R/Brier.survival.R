### Brier.survival.R ---
#----------------------------------------------------------------------
## Author: Thomas Alexander Gerds
## Created: Jan 11 2022 (17:04)
## Version:
## Last-Updated: Jun 30 2023 (10:59) 
##           By: Thomas Alexander Gerds
##     Update #: 4
#----------------------------------------------------------------------
##
### Commentary:
##
### Change Log:
#----------------------------------------------------------------------
##
### Code:

Brier.survival <- function(DT,
                           MC,
                           se.fit,
                           conservative,
                           cens.model,
                           keep.vcov=FALSE,
                           keep.iid=FALSE,
                           multi.split.test,
                           alpha,
                           N,
                           NT,
                           NF,
                           dolist,
                           keep.residuals=FALSE,
                           IC.data,
                           ...){
    IC0=IPCW=nth.times=ID=time=times=raw.Residuals=risk=Brier=residuals=WTi=Wt=status=setorder=model=IF.Brier=data.table=sd=lower=qnorm=se=upper=NULL
    ## compute 0/1 outcome:
    DT[time<=times & status==1,residuals:=(1-risk)^2/WTi]
    DT[time<=times & status==0,residuals:=0]
    DT[time>times,residuals:=(risk)^2/Wt]
    
    if (se.fit[[1]]==1L || multi.split.test[[1]]==TRUE){
        ## data.table::setorder(DT,model,times,time,-status)
        data.table::setorder(DT,model,times,ID)
        DT[,nth.times:=as.numeric(factor(times))]
        DT[,IC0:=residuals-mean(residuals),by=list(model,times)]
        DT[,IF.Brier:=getInfluenceCurve.Brier(t=times[1],
                                              time=time,
                                              IC0,
                                              residuals=residuals,
                                              IC.G=MC,
                                              cens.model=cens.model,
                                              conservative = conservative,
                                              nth.times=nth.times[1],
                                              event = status),by=list(model,times)]
        score <- DT[,data.table(Brier=sum(residuals)/N,
                                se=sd(IF.Brier)/sqrt(N)),by=list(model,times)]
        score[,lower:=pmax(0,Brier-qnorm(1-alpha/2)*se)]
        score[,upper:=pmin(1,Brier + qnorm(1-alpha/2)*se)]
    }else{
        ## no se.fit
        score <- DT[,data.table(Brier=sum(residuals)/N),by=list(model,times)]
    }
    data.table::setkey(score,model,times)
    if (length(dolist)>0L){
        ## merge with Brier score
        data.table::setkey(DT,model,times)
        ## data.table::setkey(score,model,times)
        DT <- DT[score]
        if (se.fit[[1]]==TRUE || multi.split.test[[1]]==TRUE){
            contrasts.Brier <- DT[,getComparisons(data.table(x=Brier,IF=IF.Brier,model=model),
                                                  NF=NF,
                                                  N=N,
                                                  alpha=alpha,
                                                  dolist=dolist,
                                                  multi.split.test=multi.split.test,
                                                  se.fit=se.fit),by=list(times)]
        }else{
            contrasts.Brier <- DT[,getComparisons(data.table(x=Brier,model=model),
                                                  NF=NF,
                                                  N=N,
                                                  alpha=alpha,
                                                  dolist=dolist,
                                                  multi.split.test=FALSE,
                                                  se.fit=FALSE),by=list(times)]
        }
        setnames(contrasts.Brier,"delta","delta.Brier")
        output <- list(score=score,contrasts=contrasts.Brier)
    } else{
        output <- list(score=score)
    }
    if (keep.vcov[1] && se.fit[1]==TRUE){
        output <- c(output,list(vcov=getVcov(DT,"IF.Brier",times=TRUE)))
    }
    if (keep.iid[1] && se.fit[1] == TRUE) {
        output <- c(output,
                    list(iid.decomp = DT[,data.table::data.table(ID,model,times,IF.Brier)]))
    }
    if (keep.residuals) {
        if (all(c("Wt","WTi")%in%names(DT))){
            DT[,IPCW:=1/WTi]
            DT[time>=times,IPCW:=1/Wt]
            DT[time<times & status==0,IPCW:=0]
            output <- c(output,list(residuals=DT[,c("ID","time","status","model","times","risk","residuals","IPCW"),with=FALSE]))
        }else{
            output <- c(output,list(residuals=DT[,c("ID","time","status","model","times","risk","residuals"),with=FALSE]))
        }
    }
    output
}

#----------------------------------------------------------------------
### Brier.survival.R ends here
