% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/boxplot.Score.R
\name{boxplot.Score}
\alias{boxplot.Score}
\title{Boxplot risk quantiles}
\usage{
\method{boxplot}{Score}(
  x,
  model,
  reference,
  type = "risk",
  timepoint,
  overall = 1L,
  lwd = 3,
  xlim,
  xlab = "",
  main,
  outcome.label,
  outcome.label.offset = 0,
  event.labels,
  refline = (type != "risk"),
  add = FALSE,
  ...
)
}
\arguments{
\item{x}{Score object obtained by calling function \code{Score}.}

\item{model}{Choice of risk prediction model}

\item{reference}{Choice of reference risk prediction model for
calculation of risk differences.}

\item{type}{Either \code{"risk"} for predicted risks or
\code{"diff"} for differences between predicted risks.}

\item{timepoint}{time point specifying the prediction horizon}

\item{overall}{Logical. Tag to be documented.}

\item{lwd}{line width}

\item{xlim}{x-axis limits}

\item{xlab}{x-axis label}

\item{main}{title of plot}

\item{outcome.label}{Title label for column which shows the outcome
status}

\item{outcome.label.offset}{Vertical offset for outcome.label}

\item{event.labels}{Labels for the different events (causes).}

\item{refline}{Logical, for \code{type="diff"} only. If \code{TRUE}
draw a red vertical line at \code{0}.}

\item{add}{Logical. Tag to be documented.}

\item{...}{not used}
}
\description{
Retrospective boxplots of risk quantiles conditional on outcome
}
\examples{
# binary outcome
library(data.table)
library(prodlim)
set.seed(10)
db=sampleData(40,outcome="binary")
fitconv=glm(Y~X3+X5,data=db,family=binomial)
fitnew=glm(Y~X1+X3+X5+X6+X7,data=db,family=binomial)
x=Score(list(new=fitnew,conv=fitconv),
        formula=Y~1,contrasts=list(c(2,1)),
               data=db,plots="box",null.model=FALSE)
boxplot(x)

# survival outcome
library(survival)
ds=sampleData(40,outcome="survival")
fit=coxph(Surv(time,event)~X6+X9,data=ds,x=TRUE,y=TRUE)
\dontrun{ 
scoreobj=Score(list("Cox"=fit),
                formula=Hist(time,event)~1, data=ds,
                metrics=NULL, plots="box",
                times=c(1,5),null.model=FALSE)
boxplot(scoreobj,timepoint=5)
boxplot(scoreobj,timepoint=1)

}

# competing risks outcome
library(survival)
data(Melanoma, package = "riskRegression")
fit = CSC(Hist(time,event,cens.code="censored")~invasion+age+sex,data=Melanoma)
scoreobj=Score(list("CSC"=fit),
               formula=Hist(time,event,cens.code="censored")~1,
               data=Melanoma,plots="box",times=5*365.25,null.model=FALSE)
par(mar=c(4,12,4,4))
boxplot(scoreobj,timepoint=5*365.25)

# more than 2 competing risks
m=lava::lvm(~X1+X2+X3)
lava::distribution(m, "eventtime1") <- lava::coxWeibull.lvm(scale = 1/100)
lava::distribution(m, "eventtime2") <- lava::coxWeibull.lvm(scale = 1/100)
lava::distribution(m, "eventtime3") <- lava::coxWeibull.lvm(scale = 1/100)
lava::distribution(m, "censtime") <- lava::coxWeibull.lvm(scale = 1/100)
lava::regression(m,eventtime2~X3)=1.3
m <- lava::eventTime(m,
time ~ min(eventtime1 = 1, eventtime2 = 2, eventtime3 = 3, censtime = 0), "event")
set.seed(101)
dcr=as.data.table(lava::sim(m,101))
fit = CSC(Hist(time,event)~X1+X2+X3,data=dcr)
scoreobj=Score(list("my model"=fit),
               formula=Hist(time,event)~1,
               data=dcr,plots="box",times=5,null.model=FALSE)
boxplot(scoreobj)


}
