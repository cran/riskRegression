#' Plotting time-varying effects from a risk regression model.
#' 
#' Plot time-varying effects from a risk regression model.
#' @param x Fitted object obtained with one of \code{ARR}, \code{LRR},
#' \code{riskRegression}.
#' @param formula A formula to specify the variable(s) whose regression
#' coefficients should be plotted.
#' @param level For categorical variables the level (group) whose contrast to
#' the reference level (group) should be plotted.
#' @param ref.line Logical. If \code{TRUE} then add a horizontal line at zero.
#' @param conf.int Logical. If \code{TRUE} then add confidence limits.  Can be
#'                controlled using smart arguments. See examples
#' @param xlim See \code{plot}
#' @param ylim See \code{plot}
#' @param xlab See \code{plot}
#' @param ylab See \code{plot}
#' @param col A vector of colors for the regression coefficients.
#' @param lty A vector of line types for the regression coefficients.
#' @param lwd A vector of line thicknesses for the regression coefficients.
#' @param add Logical. If \code{TRUE} then add lines to an existing plot.
#' @param legend Logical. If \code{TRUE} then add a legend. Can be controlled
#' using smart arguments. See examples.
#' @param axes Logical. If \code{FALSE} then do not draw axes.
#' @param \dots Used for transclusion of smart arguments for \code{plot},
#' \code{axis}. See function \code{\link[prodlim]{SmartControl}} from prodlim.
#' @author Thomas H. Scheike \email{ts@@biostat.ku.dk}
#' 
#' Thomas A. Gerds \email{tag@@biostat.ku.dk}
#' @keywords survival
##' @examples
##' 
##' library(survival)
##' library(prodlim)
##' data(Melanoma)
##' 
##' fit.tarr <- ARR(Hist(time,status)~strata(sex),
##'                 data=Melanoma,
##'                 cause=1)
##' plotEffects(fit.tarr)
##' 
##' fit.tarr <- ARR(Hist(time,status)~strata(sex)+strata(invasion),
##'                 data=Melanoma,
##'                 cause=1,
##'                 times=seq(800,3000,20))
##' plotEffects(fit.tarr,formula=~sex)
##' plotEffects(fit.tarr,formula=~invasion)
##' plotEffects(fit.tarr,
##'             formula=~invasion,
##'             level="invasionlevel.1")
##' 
##' ## legend arguments are transcluded:
##' plotEffects(fit.tarr,
##'             formula=~invasion,
##'             legend.bty="b",
##'             legend.cex=1)
##' 
##' ## and other smart arguments too:
##' plotEffects(fit.tarr,
##' 	    formula=~invasion,
##' 	    legend.bty="b",
##' axis2.las=2,
##' 	    legend.cex=1)
##' 
##' 
#' @export
plotEffects <- function(x,
                        formula,
                        level,
                        ref.line=TRUE,
                        conf.int=.95,
                        xlim,
                        ylim,
                        xlab="Time",
                        ylab="Cumulative coefficient",
                        col,
                        lty,
                        lwd,
                        add=FALSE,
                        legend,
                        axes=TRUE,
                        ...){
  # {{{ find variables and coefficients with confidence limits

    timevars <- x$design$timevar
    if (length(timevars)==0) stop("No variable with time-varying effect in model design.")
    if (missing(formula))
        thisvar <- timevars[1]
    else 
        thisvar <- all.vars(formula)[1]
    time <- x$timeVaryingEffects$coef[,"time"]
    matchVar <- grep(thisvar,colnames(x$timeVaryingEffects$coef))
    matchVarnames <- grep(thisvar,colnames(x$timeVaryingEffects$coef),value=TRUE)
    coef <- x$timeVaryingEffects$coef[,matchVar,drop=FALSE]
    se <- sqrt(x$timeVaryingEffects$var[,matchVar,drop=FALSE])
    zval <- qnorm(1- (1-conf.int)/2, 0,1)
    lower <- coef-zval*se
    upper <- coef + zval*se
    # select levels for categorical variables
    levs <- colnames(coef)
    if (!missing(level)) {
        levs <- levs[match(level,levs,nomatch=0)]
        matchVarnames <- matchVarnames[match(levs,matchVarnames,nomatch=0)]
    }
    if (length(levs)==0) stop(paste("Could not find level(s): ",paste(level,collapse=", ")),"\nAvailable levels: ",paste(colnames(coef),collapse=", "))
    # }}}
    # {{{  plotting limits, colors, etc
    if (missing(ylim))
        ylim <- c(floor(min(lower,na.rm=1L)),ceiling(max(upper,na.rm=1L)))
    if (missing(xlim))
        xlim=c(min(time),max(time))
    if (missing(col))
        col <- 1:12
    col <- rep(col,length.out=NCOL(coef))
    if (missing(lty))
        lty <- 1
    lty <- rep(lty,length.out=NCOL(coef))
    if (missing(lwd))
        lwd <- 2
    lwd <- rep(lwd,length.out=NCOL(coef))
    # }}}
    # {{{  setting default arguments
    ## if (missing(legend)) legend <- length(matchVar)>1
    if (missing(legend)) legend <- TRUE
    background.DefaultArgs <- list(xlim=xlim,ylim=ylim,horizontal=seq(0,1,.25),vertical=NULL,bg="white",fg="gray88")
    axis1.DefaultArgs <- list()
    axis2.DefaultArgs <- list(side=2)
    lines.DefaultArgs <- list(type="s")
    plot.DefaultArgs <- list(x=0,y=0,type = "n",ylim = ylim,xlim = xlim,xlab = xlab,ylab = ylab)
  
  legend.DefaultArgs <- list(legend=matchVarnames,
                             trimnames=FALSE,
                             lwd=lwd,
                             col=col,
                             lty=lty,
                             cex=1.5,
                             bty="n",
                             y.intersp=1.3,
                             x="topright")
  ## conf.int.DefaultArgs <- list(x=x,newdata=newdata,type=type,citype="shadow",times=plot.times,cause=cause,density=55,col=col[1:nlines],lwd=rep(2,nlines),lty=rep(3,nlines))
  # }}}
  # {{{ smart control

  smartA <- prodlim::SmartControl(call=  list(...),
                         keys=c("plot","legend","conf.int","axis1","axis2"),
                         ignore=c("x","formula","ref.line","add","col","lty","lwd","ylim","xlim","xlab","ylab","legend","conf.int","axes"),
                         defaults=list("plot"=plot.DefaultArgs,
                           ## "lines"=lines.DefaultArgs,
                           "legend"=legend.DefaultArgs,
                           ## "conf.int"=conf.int.DefaultArgs,
                           ## "background"=background.DefaultArgs,
                           "axis1"=axis1.DefaultArgs,
                           "axis2"=axis2.DefaultArgs),
                         forced=list("plot"=list(axes=FALSE),"axis1"=list(side=1)),
                         ignore.case=TRUE,
                         replaceDefaults=FALSE,
                         verbose=TRUE)

  # }}}
  # {{{  plot and backGround
  if (!add) {
    do.call("plot",smartA$plot)
  }
  # }}}
  # {{{  axes

  if (!add) {
    if (axes){
      do.call("axis",smartA$axis1)
      do.call("axis",smartA$axis2)
    }
  }

  # }}}
  # {{{ adding the lines

  if (length(matchVar)>1){
    ref <- x$refLevels[thisvar]
    if (ref.line==TRUE)
      abline(h=0,col="gray55",lwd=2)
    nix <- lapply(1:length(levs),function(l){
      i <- match(levs[l],colnames(coef),nomatch=0)
      lines(time,coef[,i],col=col[l],lwd=lwd[l],lty=lty[l],type="s")
      ## confidence shadows
      ccrgb=as.list(col2rgb(col[l],alpha=TRUE))
      names(ccrgb) <- c("red","green","blue","alpha")
      ccrgb$alpha=55
      cc=do.call("rgb",c(ccrgb,list(max=255)))
      polygon(x=c(time,rev(time)),y=c(lower[,i],rev(upper[,i])),col=cc,border=NA)
    })
  }
  else{
    if (ref.line==TRUE)
      abline(h=0,col="gray55",lwd=2)
    lines(time,coef,lwd=lwd[1],lty=lty[1],col=col[1],type="s")
    ## confidence shadows
    ccrgb=as.list(col2rgb(col[1],alpha=TRUE))
    names(ccrgb) <- c("red","green","blue","alpha")
    ccrgb$alpha=55
    cc=do.call("rgb",c(ccrgb,list(max=255)))
    polygon(x=c(time,rev(time)),y=c(lower,rev(upper)),col=cc,border=NA)
  }

  # }}}
    # {{{  legend

  if(legend[[1]]==TRUE && !add[[1]] && !is.null(matchVarnames)){

    ## if (smartA$legend$trimnames==TRUE){
    ## smartA$legend$legend <- sapply(strsplit(names(Y),"="),function(x)x[[2]])
    ## }
    smartA$legend <- smartA$legend[-match("trimnames",names(smartA$legend))]
    save.xpd <- par()$xpd
    par(xpd=TRUE)
    do.call("legend",smartA$legend)
    par(xpd=save.xpd)
  }
  # }}}

}
