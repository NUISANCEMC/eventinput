from ._pyNUISANCE import EventFrameGen, EventFrame
from ._pyNUISANCE import HistFrame, BinnedValues

import matplotlib.pyplot as plt
import numpy as np

class EventFrameGen_mpl_helper():
    def __init__(self, fr):
        self.fr = fr

    def scatter(self, x, y, c=None, *args, **kwargs):
        df = self.fr.all()
        if c:
            c = df[c]
            kwargs["c"] = c
        obj = plt.scatter( df[x], df[y], *args, **kwargs )
        return obj
    
    def hist(self, x, y, weights="cvw", *args, **kwargs):
        df = self.fr.all()
        obj = plt.hist( df[x], self.df[weights], *args, **kwargs )
        return obj
    
    def hist2d(self, x, y, weights="cvw", *args, **kwargs):
        df = self.fr.all()
        obj = plt.hist2d( df[x], df[y], self.df[weights], *args, **kwargs )
        return obj

class EventFrame_mpl_helper():
    def __init__(self, df):
        self.df = df

    def scatter(self, x, y, *args, **kwargs):
        obj = plt.scatter( self.df[x], self.df[y], *args, **kwargs )
        return obj
    
    def hist(self, x, weights="cvw", *args, **kwargs):
        obj = plt.hist( self.df[x], weights=self.df[weights], *args, **kwargs )
        return obj
    
    def hist2d(self, x, y, weights="cvw", *args, **kwargs):
        print(self.df, x, y)
        obj = plt.hist2d( self.df[x], self.df[y], weights=self.df[weights], *args, **kwargs )
        return obj



class HistFrame_matplotlib_helper:
    def __init__(self, hf):
        self.hf = hf

    def fc(self):
        return self.hf.column_info[0].name
        
    def x(self): return [(x[0].low+x[0].high)/2 for x in self.hf.binning.bins]
    def ex(self): return [(x[0].high-x[0].high)/2 for x in self.hf.binning.bins]
    def lx(self): return self.hf.binning.axis_labels[0]
    def bx(self): return ([(x[0].low) for x in self.hf.binning.bins]
                + [self.hf.binning.bins[-1][0].high])
    def by(self): return ([(x[1].low) for x in self.hf.binning.bins]
                + [self.hf.binning.bins[-1][1].high])
        
        
    def y(self): return [(x[1].low+x[1].high)/2 for x in self.hf.binning.bins]
    def ey(self): return [(x[1].high-x[1].high)/2 for x in self.hf.binning.bins]
    def ly(self): return self.hf.binning.axis_labels[1]
    
    def  c(self, col=None): 
        if not col: col = self.fc()
        if "count" in self.hf[col]: return self.hf[col]["count"]
        if "value" in self.hf[col]: return self.hf[col]["value"]

    def ec(self, col=None): 
        if not col: col = self.fc()
        if "variance" in self.hf[col]: return np.sqrt(self.hf[col]["variance"])
        if "error" in self.hf[col]: return self.hf[col]["error"]

    def form_title(self, col, **kwargs):
        if "label" not in kwargs:
            kwargs["label"] = col
        return kwargs

    def get_1d_plotdim(self, axis):
        if axis == "x": 
            pdim = self.x()
            perr = self.ex()
            plab = self.lx()
        if axis == "y": 
            pdim = self.y()
            perr = self.ey()
            plab = self.ly()
        if axis == "z": 
            pdim = self.z()
            perr = self.ez()
            plab = self.lz()
        return pdim, perr, plab

    def get_1d_bins(self, axis):
        if axis == "x": 
            return ([(x[0].low) for x in self.hf.binning.bins]
                + [self.hf.binning.bins[-1][0].high])
            
    def errorbar(self, axis="x", col=None, *args, **kwargs):
        pdim, perr, plab = self.get_1d_plotdim(axis)
        obj = plt.errorbar(pdim, self.c(col), xerr=perr, 
                            yerr=self.ec(), *args, **kwargs )
        plt.xlabel(plab)
        return obj

    def plot(self, axis="x", col=None, fill=None, *args, **kwargs):
        pdim, perr, plab = self.get_1d_plotdim(axis)
        obj = plt.plot( pdim, self.c(col), *args, **kwargs )
        if fill == "tozeroy":
            plt.fill_between(pdim, np.zeros(len(pdim)), self.c(col))
        plt.xlabel(plab)
        return obj
    
    def plot_all(self, axis="x", cols=None, labels=None, *args, **kwargs):
        if not cols: cols = [x.name for x in self.hf.column_info]
        if not labels: labels = cols
        for col in cols:
            self.plot(axis, col, *args, **kwargs)
        return 

    def fill(self, axis="x", col="mc", fill=None, *args, **kwargs):
        pdim, perr, plab = self.get_1d_plotdim(axis)
        obj = plt.fill_between(pdim, np.zeros(len(pdim)), self.c(col), *args, **kwargs )
        return obj
        
    def fill_all(self, axis="x", cols=None, labels=None, *args, **kwargs):
        if not cols: cols = [x.name for x in self.hf.column_info]
        if not labels: labels = cols
        for col, label in zip(cols,labels):
            kwargs["label"] = label
            self.fill(axis, col, *args, **kwargs)
        return 

    def scatter(self, axis="x", col=None, *args, **kwargs):
        pdim, perr, plab = self.get_1d_plotdim(axis)
        obj = plt.scatter( pdim, self.c(col), *args, **kwargs )
        plt.xlabel(plab)
        return obj

    def scatter_all(self, axis="x", cols=None, labels=None, *args, **kwargs):
        if not cols: cols = self.hf.column_info
        if not labels: labels = cols
        for col in cols:
            self.scatter(axis, col, *args, **kwargs)
        return 
    
    def hist(self, axis="x", col=None, *args, **kwargs):
        pdim, perr, plab = self.get_1d_plotdim(axis)
        obj = plt.hist( pdim, weights=self.c(col), bins=self.get_1d_bins(axis), *args, **kwargs )
        plt.xlabel(plab)
        return obj
    
    def hist_all(self, axis="x", cols=None, labels=None, *args, **kwargs):
        if not cols: cols = [x.name for x in self.hf.column_info]
        if not labels: labels = cols
        for col, label in zip(cols,labels):
            kwargs["label"] = label
            self.hist(axis, col, *args, **kwargs)
        return 
 

    def hist2d(self, xaxis="x", yaxis="y", col=None, *args, **kwargs):
        pdim, perr, plab = self.get_1d_plotdim(xaxis)
        pdim2, perr2, plab2 = self.get_1d_plotdim(yaxis)
        obj = plt.hist2d( pdim, pdim2, weights=self.c(col), bins=[np.unique(self.bx()), np.unique(self.by())], *args, **kwargs )
        plt.xlabel(plab)
        plt.ylabel(plab2)
        
        return obj
    
    def colormesh(self, xaxis="x", yaxis="y", col=None, *args, **kwargs):

        # Placeholder for noow will update to arb axis
        nbins = len(self.hf.binning.bins)
        X = np.zeros((2*nbins,2))
        Y = np.zeros((2*nbins,2))
        C = np.zeros(((2*nbins)-1,1))
        for i,bin in enumerate(self.hf.binning.bins):
            X[2*i,0] = bin[0].low
            Y[2*i,0] = bin[1].low
            
            X[2*i + 1,0] = bin[0].low
            Y[2*i + 1,0] = bin[1].high
            
            X[2*i, 1] = bin[0].high
            Y[2*i, 1] = bin[1].low

            X[2*i + 1, 1] = bin[0].high
            Y[2*i + 1, 1] = bin[1].high

            C[2*i,0] = self.hf.sumweights[i,0]
            if (2*i + 2) != (2*nbins):
                C[2*i + 1,0] = self.hf.sumweights[i,0]
                
        plt.pcolormesh(X, Y, C)

def mpl_cern_template(page_dim=[3,3]):
    plt.minorticks_on()
    plt.rcParams.update({
        'figure.figsize': page_dim,
        'xtick.direction': 'in',
        'ytick.direction': 'in',
        'xtick.top': True,
        'ytick.right': True,
        'xtick.major.size': 6,
        'ytick.major.size': 6,
        'xtick.minor.size': 3,
        'ytick.minor.size': 3,
        'xtick.major.width': 1,
        'ytick.major.width': 1,
        'xtick.minor.width': 1,
        'ytick.minor.width': 1,
        'xtick.labelsize': 10,
        'ytick.labelsize': 10,
        'axes.labelsize': 12,
        'axes.titlesize': 14,
        'font.family': 'sans-serif',  # Set font family
        'font.serif': ['Times New Roman'],  # Set font to Times New Roman
        'axes.linewidth': 1.1,  # Increase axis linewidth for better visibility
        'axes.grid': False,  # Enable grid
        'axes.grid.which': 'major',  # Show both major and minor grid lines
        'axes.grid.axis': 'both',  # Show grid lines on both x and y axes
        'figure.dpi': 400,
        'legend.frameon': True,
        'legend.facecolor': 'none',  # Set legend background color
        'legend.edgecolor': 'none',  # Set legend border color to transparent
    })
    

#Dynamic Patching
def build_HistFrame_matplotlib_helper(self):
    return HistFrame_matplotlib_helper(self)

def build_EventFrameGen_mpl_helper(self):
    return EventFrameGen_mpl_helper(self)

def build_EventFrame_mpl_helper(self):
    return EventFrame_mpl_helper(self)

EventFrameGen.mpl = build_EventFrameGen_mpl_helper
HistFrame.mpl = build_HistFrame_matplotlib_helper
<<<<<<< HEAD
BinnedValues.mpl = build_HistFrame_matplotlib_helper
EventFrame.mpl = build_EventFrame_mpl_helper
=======
EventFrame.mpl = build_Frame_mpl_helper
>>>>>>> origin/main

HistFrame.show = plt.show

# naughty addition to mpl
cern = mpl_cern_template
