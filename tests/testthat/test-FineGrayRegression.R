library(testthat)
context("Fine-Gray regression")
library(cmprsk)
library(riskRegression)
library(data.table)
library(prodlim)
test_that("Formula interface",{
    set.seed(17)
    d <- prodlim::SimCompRisk(100)
    a <- FGR(Hist(time,event)~X1+X2,data=d)
    b <- cmprsk::crr(ftime=d$time,fstatus=d$event,cov1=model.matrix(terms(~X1+X2),data=d)[,-1])
    ## remove call
    b <- b[-match("call",names(b))]
    class(b) <- "crr"
    expect_equal(ignore_attr=TRUE,a$crrFit,b)
})
if(FALSE){
    test_that("Functions of time",{
        qFun <- function(x){x^2}
        id <- function(x){x}
        set.seed(17)
        d <- prodlim::SimCompRisk(100)
        a <- FGR(Hist(time,event)~cov2(X1,tf=qFun)+cov2(X2),data=d)
        b <- with(d,cmprsk::crr(ftime=time,
                                fstatus=cause,
                                cov2=cbind(X1,X2),
                                tf=function(time){cbind(qFun(time),time)}))
        e <- with(d,
                  cmprsk::crr(ftime=time,
                              fstatus=cause,
                              cov2=cbind(X1,X2),
                              tf=function(x){do.call("cbind",
                                                     lapply(list("qFun", "id"), function(f) {do.call(f,list(x))}))}))
        ## remove call
        b <- b[-match("call",names(b))]
        class(b) <- "crr"
        e <- e[-match("call",names(e))]
        class(e) <- "crr"
        expect_equal(ignore_attr=TRUE,,a$crrFit,b)
        expect_equal(ignore_attr=TRUE,,a$crrFit,e)
    })
}
## expect_true(length(coef(ee))==4)



