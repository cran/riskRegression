### zzz.R --- 
#----------------------------------------------------------------------
## Author: Thomas Alexander Gerds
## Created: Apr  6 2018 (10:39) 
## Version: 
## Last-Updated: Apr  6 2018 (10:40) 
##           By: Thomas Alexander Gerds
##     Update #: 1
#----------------------------------------------------------------------
## 
### Commentary: 
## 
### Change Log:
#----------------------------------------------------------------------
## 
### Code:

.onLoad <- function(libname, pkgname) {
    vig_list = tools::vignetteEngine(package = 'knitr')
    vweave <- vig_list[['knitr::knitr']][c('weave')][[1]]
    vtangle <- vig_list[['knitr::knitr']][c('tangle')][[1]]
    tools::vignetteEngine(pkgname, weave = vweave, tangle = vtangle,
                          pattern = "[.]Rmd$", package = pkgname)
    #register_vignette_engines(pkgname)
}

######################################################################
### zzz.R ends here
