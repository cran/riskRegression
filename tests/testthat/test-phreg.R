### test-phreg.R --- 
#----------------------------------------------------------------------
## author: Brice Ozenne
## created: feb 28 2017 (09:52) 
## Version: 
## last-updated: Sep 17 2022 (07:02) 
##           By: Thomas Alexander Gerds
##     Update #: 31
#----------------------------------------------------------------------
## 
### Commentary: 
## 
### Change Log:
#----------------------------------------------------------------------
## 
### Code:

## * Settings
library(testthat)
library(riskRegression)
library(rms)
library(survival)
library(prodlim)
library(mets)

context("Compatibility with the phreg function from the mets package")

## * Survival

## ** Data
set.seed(10)
d <- sampleData(5e1, outcome = "survival")[,.(eventtime,event,X1,X2,X3,X4,X6)]
d[ , Xcat2 := as.factor(paste0(X1,X2))]
d[, entry:= 0]
d[, id := 1:.N]
d[, group := factor(rbinom(.N,2,0.5))]

## ** Tests
test_that("survival - no strata", {
    m.phreg <- phreg(Surv(entry,eventtime,event)~X1+X2,data=d)
    m.coxph <- coxph(Surv(eventtime,event)~X1+X2,data=d, x = TRUE, y = TRUE) ## do not center anymore factor or binary variables
    m.cph <- cph(Surv(eventtime,event)~X1+X2,data=d, x = TRUE, y = TRUE)
    ## slight difference in estimated coefficients: coef(m.phreg) - coef(m.cph)
    
    expect_equal(ignore_attr=TRUE,predictCox(m.phreg, center = FALSE),predictCox(m.coxph, center = FALSE), tolerance = 1e-8)
    expect_equal(ignore_attr=TRUE,predictCox(m.phreg, center = TRUE),predictCox(m.cph, center = TRUE), tolerance = 1e-4)

    pred.phreg <- predictCox(m.phreg, newdata = d, time = 1:5, se = TRUE)
    pred.coxph <- predictCox(m.coxph, newdata = d, time = 1:5, se = TRUE)
    
    expect_equal(ignore_attr=TRUE,pred.phreg$survival, pred.coxph$survival, tolerance = 1e-8)
})

test_that("survival - one strata variable", {
    mS.phreg <- phreg(Surv(entry,eventtime,event)~X1+X2+strata(group)+cluster(id),data=d)
    mS.coxph <- coxph(Surv(eventtime,event)~X1+X2+strata(group),data=d, x = TRUE, y = TRUE) ## do not center anymore factor or binary variables
    mS.cph <- cph(Surv(eventtime,event)~X1+X2+strat(group),data=d, x = TRUE, y = TRUE)
    ## slight difference in estimated coefficients: coef(mS.phreg) - coef(mS.cph)
    
    expect_equal(ignore_attr=TRUE,predictCox(mS.phreg, center = FALSE),predictCox(mS.coxph, center = FALSE), tolerance = 1e-8)
    expect_equal(ignore_attr=TRUE,predictCox(mS.phreg, center = TRUE)[c("cumhazard","survival")],predictCox(mS.cph, center = TRUE)[c("cumhazard","survival")], tolerance = 1e-4)

    predS.phreg <- predictCox(mS.phreg, newdata = d, time = 1:5, se = TRUE)
    predS.coxph <- predictCox(mS.coxph, newdata = d, time = 1:5, se = TRUE)
    expect_equal(ignore_attr=TRUE,predS.phreg$survival, predS.coxph$survival, tolerance = 1e-8)

    ## bug in previous version, could not create design matrix when group had only one value
    predS.phreg <- predictCox(mS.phreg, newdata = d[group==0], time = 1:5, se = TRUE)
    predS.coxph <- predictCox(mS.coxph, newdata = d[group==0], time = 1:5, se = TRUE)
    expect_equal(ignore_attr=TRUE,predS.phreg$survival, predS.coxph$survival, tolerance = 1e-8)

})


## several strata variables
test_that("survival - several strata variables", {
    mS.phreg <- phreg(Surv(entry,eventtime,event)~X1+X2+strata(group,X3)+cluster(id),data=d)
    mS.coxph <- coxph(Surv(eventtime,event)~X1+X2+strata(group)+strata(X3),data=d, x = TRUE, y = TRUE) ## do not center anymore factor or binary variables
    mS.cph <- cph(Surv(eventtime,event)~X1+X2+strat(group)+strat(X3),data=d, x = TRUE, y = TRUE)
    ## slight difference in estimated coefficients: coef(mS.phreg) - coef(mS.cph)

    expect_equal(ignore_attr=TRUE,predictCox(mS.phreg, center = FALSE)[c("cumhazard","survival")],
                 predictCox(mS.coxph, center = FALSE)[c("cumhazard","survival")],
                 tolerance = 1e-8)

    ## FAIL BECAUSE DIFFERENT ORDERING OF THE STRATA
    ## expect_equal(ignore_attr=TRUE,predictCox(mS.phreg, center = TRUE)[c("cumhazard","survival")],
    ##              predictCox(mS.cph, center = TRUE)[c("cumhazard","survival")],
    ##              tolerance = 1e-4)

    predS.phreg <- predictCox(mS.phreg, newdata = d, time = 1:5, se = TRUE)
    predS.coxph <- predictCox(mS.coxph, newdata = d, time = 1:5, se = TRUE)
    expect_equal(ignore_attr=TRUE,predS.phreg$survival, predS.coxph$survival, tolerance = 1e-8)


})

## * Competing risks
## ** Data
set.seed(10)
d <- sampleData(1e2, outcome = "competing.risks")[,.(time,event,X1,X2,X3,X4,X6)]
d[ , Xcat2 := as.factor(paste0(X1,X2))]
d[, id := 1:.N]
d[, group := factor(rbinom(.N,2,0.5))]

## ** Tests
test_that("competing risk - no strata", {
    m.phreg <- CSC(Hist(time, event)~X1+X2,data=d,
                   fitter = "phreg")
    m.coxph <- CSC(Hist(time,event)~X1+X2,data=d,
                   fitter = "coxph")
    expect_equal(ignore_attr=TRUE,class(m.phreg$models[[1]]),"phreg")
    pred.phreg <- predict(m.phreg, newdata = d, times = 1:5, cause = 1, se = TRUE)
    pred.coxph <- predict(m.coxph, newdata = d, times = 1:5, cause = 1, se = TRUE)
    expect_equal(ignore_attr=TRUE,pred.phreg,pred.coxph, tolerance = 1e-8)
})

test_that("competing risk - one strata variable", {
    mS.phreg <- CSC(Hist(time, event)~X1+X2+strata(group)+cluster(id),data=d,
                    fitter = "phreg")
    mS.coxph <- CSC(Hist(time,event)~X1+X2+strata(group),data=d,
                    fitter = "coxph")

    expect_equal(ignore_attr=TRUE,class(mS.phreg$models[[1]]),"phreg")
    predS.phreg <- predict(mS.phreg, newdata = d, times = 1:5, cause = 1, se = TRUE)
    predS.coxph <- predict(mS.coxph, newdata = d, times = 1:5, cause = 1, se = TRUE)
    expect_equal(ignore_attr=TRUE,predS.phreg,predS.coxph, tolerance = 1e-3)
})

test_that("competing risk - several strata variables", {
    mS.phreg <- CSC(Hist(time, event)~X1+X2+strata(group,X3)+cluster(id),data=d,
                    fitter = "phreg")
    mS.coxph <- CSC(Hist(time,event)~X1+X2+strata(group)+strata(X3),data=d,
                    fitter = "coxph")
    expect_equal(ignore_attr=TRUE,class(mS.phreg$models[[1]]),"phreg")
    predS.phreg <- predict(mS.phreg, newdata = d, times = 1:5, cause = 1, se = TRUE)
    predS.coxph <- predict(mS.coxph, newdata = d, times = 1:5, cause = 1, se = TRUE)
    expect_equal(ignore_attr=TRUE,predS.phreg,predS.coxph, tolerance = 1e-3)

})
##----------------------------------------------------------------------
### test-phreg.R ends here
