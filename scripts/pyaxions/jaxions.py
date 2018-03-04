#!/usr/bin/python3

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import math
import re, os
import h5py
import datetime
import glob

# mark=f"{datetime.datetime.now():%Y-%m-%d}"
# from uuid import getnode as get_mac
# mac = get_mac()
#
# from matplotlib import rc
# rc('font',**{'family':'sans-serif','sans-serif':['Helvetica']})
# ## for Palatino and other serif fonts use:
# #rc('font',**{'family':'serif','serif':['Palatino']})
# rc('text', usetex=True)
# plt.rc('font', family='serif')

# ------------------------------------------------------------------------------
#   h5 files
# ------------------------------------------------------------------------------






#   finds all axion.m.XXXXX files under an address

def findmfiles(address='./'):
    if address == '':
        address = './'
    else:
        address += '/'
    list = []
    for filename in glob.iglob(address+'**/axion.m.*', recursive=True):
          list.append(filename)
          #print(filename)
    return sorted(list) ;






#   finds m folders under an address, returns first if more than 1

def findmdir(address='./'):
    # searches for the m directory
    # print('Seach in ', address)
    if address == '':
        address = './'
    else:
        address += '/'
    list = []
    for filename in glob.iglob(address+'/**/m/', recursive=True):
          list.append(filename)
          #print(filename)
    if len(list) ==1:
        ad = list[0]
    elif len(list) == 0:
        print('Error')
        ad = ''
    else :
        print(list)
        print('Error: multiple m directories, returning first')
        ad = list[0]
    return ad;






#   moves 10000 and 100001 files out of m folders down to parent directory

def mv10001(address='./'):
    mdir = findmdir(address)
    odir = mdir[:-2]
    ch = 0
    if os.path.exists(mdir+'axion.m.10000'):
        os.rename(mdir+'axion.m.10000',odir+'axion.m.10000')
        print('moved 10000')
        ch += 1
    if os.path.exists(mdir+'axion.m.10001'):
        os.rename(mdir+'axion.m.10001',odir+'axion.m.10001')
        print('moved 10001')
        ch += 1
    if ch == 0 :
        if not os.path.exists(odir+'axion.m.10000'):
            print('No files found, argument should be a folder containing -r a m/ directory')
    return ;






#   displays attributes of a measurement file

def aximcontent(address='./'):
    f = h5py.File(address, 'r')
    # displays the atribbutes of a file
    print('Attributes of file ',f)
    for item in f.attrs.keys():
        print(item + ":", f.attrs[item])
    print()
    print('[/ic/]')
    for item in f['/ic/'].attrs.keys():
        print("     ",item + ":", f['/ic/'].attrs[item])
    print()
    print('[/potential/]')
    for item in f['/potential/'].attrs.keys():
        print("     ",item + ":", f['/potential/'].attrs[item])
    print()

    # displays the data sets?
    # returns a lists of flags?
    return ;





#   main function to extract data from axion.m.XXXXX files by concept
#

def gm(address,something='help'):

    # the help
    if something == 'help':
        print('---------------------------------------------')
        print('gm help           ')
        print('---------------------------------------------')
        print('ftype       Saxion/Axion      ')
        print('ct/z        conformal time    ')
        print('Size        Number of lattice points along 1D ')
        print('L           Phyiscal Box Length [ADM u.]     ')
        print('massA       Axion mass    [ADM u.]     ')
        print('massS       Saxion mass   [ADM u.]    ')
        print('msa         Saxion mass*L/N           ')
        print('eA          Energy Axion  [ADM u.]   ')
        print('eS          Energy SAxion [ADM u.]   ')
        print('eGA         Grad En Axion [ADM u.]   ')
        print('eGxA        Grad x En Axion [ADM u.]   ')
        print('eKA         Kin En Axion [ADM u.]   ')
        print('eVA         Pot En Axion [ADM u.]   ')
        print('stringN     String #points ')
        print('stwallN     Walls  #points   ')
        print('stDens      String length/Volume [ADM u.]   ')
        print('binconB     binned normalised log10 contrast         ')
        print('binconBmax  maximum log10(contrast)         ')
        print('binconBmin  maximum log10(contrast)         ')
        print('binthetaB   binned normalised theta value        ')
        print('binthetaBmax, binthetaBmin...   ')
        print('kmax        maximum momentum [int] in the 3D grid ')
        print('         ')
        print('nsp         binned number spectrum [total]')
        print('nspK        binned number spectrum [Kinetic energy part]')
        print('nspG        binned number spectrum [Gradient energy part]')
        print('nspV        binned number spectrum [Potential energy part]')
        print('         ')
        print('psp         binned power spectrum')
        print('         ')
        print('mapmC       2D slice map of conformal PQ field')
        print('mapvC       2D slice map of conformal PQ velocity field')
        print('         ')
        print('maptheta    2D slice map of THETA')
        print('mapvheta    2D slice map of THETA_v')
        print('mapEdens    2D slice map of ENERGY in THETA [currentlt only Axion]')
        return ;

    f = h5py.File(address, 'r')

    #prelim checks
    ftype = f.attrs.get('Field type').decode()

    # if loop
    if (something == 'ftype'):
        return ftype ;

    if (something == 'ct') or (something == 'z'):
        return f.attrs[u'z'] ;
    if something == 'Size':
        return f.attrs[u'Size'] ;
    if something == 'L':
        return f.attrs[u'Physical size'] ;
    if something == 'nqcd':
        if 'nQcd' in f:
            nqcd = f.attrs[u'nQcd']
        elif '/potential/' in f:
            if 'nQcd' in f['/potential/'].attrs:
                nqcd = f['/potential/'].attrs[u'nQcd']
            #print('new format!')
        else :
            nqcd = 7.0
        return nqcd ;
    if something == 'delta':
        L = f.attrs[u'Physical size']
        N = f.attrs[u'Size']
        return L/N ;
    if something == 'massA':
        return f.attrs[u'Axion mass'] ;
    if something == 'msa':
        return f.attrs[u'Saxion mass'] ;
    if something == 'lambda':
        typeL = f['/potential/'].attrs['Lambda type']
        if typeL == b'z2' :
            z = f.attrs[u'z']
            return f['/potential/'].attrs[u'Lambda']/(z**2) ;
        else :
            return f['/potential/'].attrs[u'Lambda'] ;
    if something == 'massS':
        L = f.attrs[u'Physical size']
        N = f.attrs[u'Size']
        delta = L/N
        msa = f.attrs[u'Saxion mass'] ;
        return msa/delta ;

    # energies or other stuff
    en_check = 'energy' in f
    if (something[0] == 'e') and en_check :
        fi = something[-1]
        if fi == 'A':
            fis = 'A'
        elif fi == 'S':
            if ftype == 'Saxion':
                fis = 'Sa'
            else:
                # print('[gm] Warning: file contains no Saxion energy!, set to 0.')
                return 0. ;
        else :
            print('[gm] what field Energy you wants?')
            return 0. ;

        if (something == 'eGx'+fi):
            return f['energy'].attrs[fis+'xion Gr X'] ;
        if (something == 'eGy'+fi):
            return f['energy'].attrs[fis+'xion Gr Y'] ;
        if (something == 'eGz'+fi):
            return f['energy'].attrs[fis+'xion Gr Z'] ;
        if (something == 'eG'+fi):
            eni = f['energy'].attrs[fis+'xion Gr X']
            eni += f['energy'].attrs[fis+'xion Gr Y']
            eni += f['energy'].attrs[fis+'xion Gr Z']
            return eni ;
        if (something == 'eK'+fi):
            return f['energy'].attrs[fis+'xion Kinetic'] ;
        if (something == 'eV'+fi):
            return f['energy'].attrs[fis+'xion Potential'] ;
        if (something == 'e'+fi):
            eni = f['energy'].attrs[fis+'xion Gr X']
            eni += f['energy'].attrs[fis+'xion Gr Y']
            eni += f['energy'].attrs[fis+'xion Gr Z']
            eni += f['energy'].attrs[fis+'xion Kinetic']
            eni += f['energy'].attrs[fis+'xion Potential']
            return eni ;
    elif (something[0] == 'e') and not en_check :
        print('[gm] No energy in the file ',address )
        return 0. ;

    # strings
    st_check = 'string' in f
    if (something[0:2] == 'st') and ftype == 'Axion':
        return 0. ;
    if (something[0:2] == 'st') and st_check and ftype == 'Saxion':
        if (something == 'stringN'):
            return f['string'].attrs[u'String number'] ;
        if (something == 'stwallN'):
            return f['string'].attrs[u'Wall number'] ;
        if (something == 'stDens'):
            stringN = f['string'].attrs[u'String number']
            L = f.attrs[u'Physical size']
            N = f.attrs[u'Size']
            ct = f.attrs[u'z']
            delta = L/N
            return  (3*delta/4)*stringN*ct*ct/(L**3) ;
    elif (something[0:2] == 'st') and not st_check :
        print('[gm] No string info in the file! Use 0.')
        return 0. ;

    # the bins
    bin_check = 'bins' in f
    if (something[0:2] == 'bi') and not bin_check :
        #print('[gm] Warning: No bins in file. Returning []')
        return ;
    if (something[0:2] == 'bi') and  bin_check :
        # contrast bin
        binconB_check = 'bins/contB' in f
        if (something[0:4] == 'binc') and not binconB_check :
            #print('[gm] Warning: No bins/contB in file. Returning []')
            return ;
        if (something[0:4] == 'binc') and binconB_check :
            if (something == 'binconB'):
                numBIN = f['bins/contB'].attrs[u'Size']
                return np.reshape(f['bins/contB/data'],(numBIN)) ;
            if (something == 'binconBmax'):
                return f['bins/contB'].attrs[u'Maximum'] ;
            if (something == 'binconBmin'):
                return f['bins/contB'].attrs[u'Minimum'] ;

        # theta bin
        bintheB_check = 'bins/thetaB' in f
        if (something[0:4] == 'bint') and not bintheB_check :
            #print('[gm] Warning: No bins/thetaB in file. Returning []')
            return ;
        if (something[0:4] == 'bint') and bintheB_check :
            if (something == 'binthetaB'):
                numBIN = f['bins/thetaB'].attrs[u'Size']
                return np.reshape(f['bins/thetaB/data'],(numBIN)) ;
            if (something == 'binthetaBmax'):
                return f['bins/thetaB'].attrs[u'Maximum'] ;
            if (something == 'binthetaBmin'):
                return f['bins/thetaB'].attrs[u'Minimum'] ;

        # rho bin
        binrhoB_check = 'bins/rhoB' in f
        if (something[0:4] == 'binr') and not binrhoB_check :
            if ftype == 'Axion' :
                print('[gm] Warning: Axion mode, no rho!')
                return ;
            elif ftype == 'Axion' :
                print('[gm] Warning: No bins/thetaB in file. Returning []')
                return ;
        if (something[0:4] == 'binr') and binrhoB_check :
            if (something == 'binrhoB'):
                numBIN = f['bins/rhoB'].attrs[u'Size']
                return np.reshape(f['bins/rhoB/data'],(numBIN)) ;
            if (something == 'binrhoBmax'):
                return f['bins/rhoB'].attrs[u'Maximum'] ;
            if (something == 'binrhoBmin'):
                return f['bins/rhoB'].attrs[u'Minimum'] ;


    if (something == 'kmax'):
        N = f.attrs[u'Size']
        return math.floor((N//2)*np.sqrt(3)+1) ;

    # the spectra

    # number spectra
    nsp_check = 'nSpectrum' in f
    if (something[0:3] == 'nsp') and not nsp_check :
        print('[gm] Warning: No nSpec in file. Returning []')
        return ;
    if (something[0:3] == 'nsp') and  nsp_check :
        powmax = f['nSpectrum/sK/data/'].size
        #ktab = (0.5+np.arange(powmax))*2*math.pi/sizeL
        if (something == 'nspK'):
            return np.reshape(f['nSpectrum/sK/data/'],(powmax)) ;
        if (something == 'nspG'):
            return np.reshape(f['nSpectrum/sG/data/'],(powmax)) ;
        if (something == 'nspV'):
            return np.reshape(f['nSpectrum/sV/data/'],(powmax)) ;
        if (something == 'nsp'):
            spec = np.reshape(f['nSpectrum/sV/data/'],(powmax)) ;
            spec += np.reshape(f['nSpectrum/sG/data/'],(powmax)) ;
            spec += np.reshape(f['nSpectrum/sK/data/'],(powmax)) ;
            return spec ;

    # power spectra
    psp_check = 'pSpectrum' in f
    if (something[0:3] == 'psp') and not psp_check :
        print('[gm] Warning: No pSpec in file. ')
        return ;
    if (something[0:3] == 'psp') and  psp_check :
        powmax = f['pSpectrum/sP/data/'].size
        if (something == 'psp'):
            return np.reshape(f['pSpectrum/sP/data/'],(powmax)) ;

    # maps
    map_check = 'map' in f
    if (something[0:3] == 'map') and not map_check :
        print('[gm] Warning: No map in file!. ')
        return ;
    if (something[0:3] == 'map') and  map_check :
        N = f.attrs[u'Size']
        ct = f.attrs[u'z']
        if (something == 'mapmC') and (ftype == 'Saxion'):
            return np.reshape(f['map']['m'].value.reshape(N,N,2)) ;
        if (something == 'mapmC') and (ftype == 'Axion'):
            return ;
        if (something == 'mapvC') and (ftype == 'Saxion'):
            return np.reshape(f['map']['v'].value.reshape(N,N,2)) ;
        if (something == 'mapvC') and (ftype == 'Axion'):
            return ;
        if (something == 'maptheta') and (ftype == 'Saxion'):
            temp = np.array(f['map']['m'].value.reshape(N,N,2))
            temp = np.arctan2(temp[:,:,1], temp[:,:,0])
            return temp ;
        if (something == 'maptheta') and (ftype == 'Axion'):
            temp = np.array(f['map']['m'].value.reshape(N,N))
            return temp/ct ;
        if (something == 'mapvheta') and (ftype == 'Axion'):
            temp = np.array(f['map']['v'].value.reshape(N,N))
            return temp ;
        if (something == 'mapEdens') and (ftype == 'Axion'):
            theta = np.array(f['map']['m'].value.reshape(N,N))/ct
            massA2 = f.attrs[u'Axion mass']
            massA2 *= massA2
            mapa = massA2*2*np.sin(theta/2)**2
            kine = np.array(f['map']['v'].value.reshape(N,N))
            mapa += ((kine - theta/ct)**2)/(2*ct*ct)
            return mapa ;

    # the irrelevants
    if something == 'Depth':
        return f.attrs[u'Depth'] ;

    print('Argument not recognised/found!')
    return ;






# ------------------------------------------------------------------------------
#   n modes for normalising power spectra (move to other file?)
# ------------------------------------------------------------------------------

from math import exp, log10, fabs, atan, log, atan2






#   volume of a box size 1 contained inside a sphere of radius rR

def volu( rR ):

    if rR <= 1.0:
        return (4*math.pi/3)*rR**3 ;

    elif 1.0 < rR <= math.sqrt(2.):
        return (2*math.pi/3)*(9*rR**2-4*rR**3-3) ;

    elif math.sqrt(2.) < rR < math.sqrt(3.):
        a2 = rR**2-2
        a = math.sqrt(a2)
        b = 8*a - 4*(3*rR**2 -1)*(atan(a)-atan(1/a))
        return b - (8/3)*(rR**3)*atan2(a*(6*rR + 4*rR**3 -2*rR**5),6*rR**4-2-12*rR**2) ;

    elif  math.sqrt(3) < rR:
        return 8. ;






#   Feturns an approximation of the number of modes
#   that we have binned as a function of |k|
#   using the previous volume function

def phasespacedensityBOXapprox ( sizeN ):

    n2 = int(sizeN//2)
    powmax = math.floor((sizeN//2)*np.sqrt(3)+1) ;
    foca = np.arange(0,powmax)/n2
    foca2 = np.arange(1,powmax+1)/n2
    vecvolu=np.vectorize(volu)
    return (n2**3)*(vecvolu(foca2)-vecvolu(foca)) ;






#   This would return the exact number of modes but it is expensive

def phasespacedensityBOXexact ( sizeN ):
    n2 = int(sizeN//2)
    powmax = math.floor((sizeN//2)*np.sqrt(3)+1) ;
    bins = np.zeros(powmax,dtype=int)
    for i in range(-n2+1,n2+1):
        for j in range(-n2+1,n2+1):
            for k in range(-n2+1,n2+1):
                mod = math.floor(math.sqrt(i*i+j*j+k*k))
                #print(i, j, k, mod)
                bins[mod] += 1
    return bins ;






#   this counts the modes starting from the highest

def phasespacedensityBOXexactHIGH ( sizeN, sizen ):
    from itertools import chain
    n2 = int(sizeN//2)
    powmax = math.floor((sizeN//2)*np.sqrt(3)+1) ;
    concatenated = chain(range(-n2+1,-n2+sizen+1), range(n2-sizen,n2+1))
    lis = list(concatenated);
    bins = np.zeros(powmax,dtype=int)
    n = 0
    for i in range(0,len(lis)):
        x = lis[i]
        for j in range(0,len(lis)):
            y = lis[j]
            for k in range(0,len(lis)):
                z = lis[k]
                mod = math.floor(math.sqrt(x*x+y*y+z*z))
                bins[mod] += 1
    return bins ;






#   My 1% approximation to the number of modes binned
#   using the exact values when the number of modes is small

def phasespacedensityBOX ( sizeN ):
    approx = phasespacedensityBOXapprox ( sizeN ) ;
    # 1% error requires 100^2 modes
    sli = 0
    for i in range(0,len(approx)):
        if approx[i] > 10000:
            sli = i
            break
    print(sli)
    exact = phasespacedensityBOXexact ( 2*sli )
    for i in range(0,sli):
        approx[i] = exact[i]

    exact = phasespacedensityBOXexactHIGH ( sizeN, 2*sli ) ;
    for i in range(len(exact)-sli,len(exact)):
        approx[i] = exact[i]

    return approx ;






#   returns a list of modes

def modelist ( sizeN ):
    n2 = int(sizeN//2)
    powmax = math.floor((sizeN//2)*np.sqrt(3)+1) ;
    lis = []
    for i in range(-n2+1,n2+1):
        for j in range(-n2+1,n2+1):
            for k in range(-n2+1,n2+1):
                mod = math.sqrt(i*i+j*j+k*k)
                lis.append(mod)
    return sorted(set(lis)) ;






# ------------------------------------------------------------------------------
#   contrast bins
# ------------------------------------------------------------------------------






#   returns a list of logarithmic bins and bin heights from a axion.m.XXXXX file
#   and a mimumum number of X points per bin by grouping
#   variable size bins

def conbin(file, X=10):
    return contrastbin(gm(file,'binconB'), gm(file,'binconBmin'), gm(file,'binconBmax'), gm(file,'Size'), X) ;






#   returns a list of logarithmic bins and bin heights from a list
#   a minimum log, maximum log, N (N3 is the original number of points)
#   and a minimum X of points in the bin

# normalise contbin with variable width bins of minimum X points
def contrastbin(logbins, mincon, maxcon, N, X):
    numBIN = len(logbins)
    N3 = N*N*N
    bino = logbins*N3*(maxcon-mincon)/numBIN
    numbins = numBIN

    # removes first points until first bin with X points
    i0 = 0
    while bino[i0] < X:
            i0 = i0 + 1
    bino = bino[i0:]

    sum = 0
    parsum = 0
    nsubbin = 0
    minimum = 10
    lista=[]

    # JOIN BINS ALL ALONG to have a minimum of X points
    for bin in range(0,len(bino)):
        # adds bin value to partial bin
        parsum += bino[bin]
        nsubbin += 1
        if nsubbin == 1:
                # records starting bin
                inbin = bin
        if parsum < X:
            # if parsum if smaller than X we will continue
            # adding bins
            sum += 1
        else:
            enbin = bin

            low = 10**((maxcon-mincon)*(i0+inbin)/numbins + mincon)
            med = 10**((maxcon-mincon)*(i0+(inbin+enbin+1)/2)/numbins + mincon)
            sup = 10**((maxcon-mincon)*(i0+enbin+1)/numbins + mincon)
            lista.append([med,parsum/(N3*(sup-low))])

            parsum = 0
            nsubbin = 0

    return np.array(lista)         ;






#   builds the cumulative distribution







# ------------------------------------------------------------------------------
#   normalises power spectra
# ------------------------------------------------------------------------------






#   takes a binned |FT|^2 of the DENSITY
#   divides by number of modes and normalises it
#   to be the logarithmic variance
#   note that our code normalises the |FT|^2 of the DENSITY
#   with a factor L^3/2*N^6 (that we also use for the nSpectrum)
#   the 1/2 is the 1/2 of kinetic, gradient and mass term ;-)
#   Since we want to multiply by k^3/2pi^2 to make the dimensionless variance,
#   we only use k^3/pi^2 for the 1/2 is already there since the code

def normalisePspectrum(psp, nmodes, avdens, N, L):
    kmax   = len(psp)
    klist  = (0.5+np.arange(kmax))*math.pi/L
    norma  = 1/((math.pi**2)*(avdens**2))
    return norma*(klist**3)*psp/nmodes;






# ------------------------------------------------------------------------------
#   higher level wrappers
# ------------------------------------------------------------------------------






#   outputs the string evolution

def stringo(mfiles):
    stringo = []
    for f in mfiles:
        if gm(f,'ftype') == 'Saxion':
            stringo.append([gm(f,'ct'),gm(f,'stDens')])
    return np.array(stringo) ;






#   outputs the energy evolution

def energo(mfiles):
    eevol = []
    for f in mfiles:
        eevol.append([gm(f,'ct'),gm(f,'eA'),gm(f,'eS')])
    return np.array(eevol) ;






#   outputs the energy evolution of axion components

def energoA(mfiles):
    eevol = []
    for f in mfiles:
        eevol.append([gm(f,'ct'),gm(f,'eKA'),gm(f,'eGA'),gm(f,'eVA'),gm(f,'eA')])
    return np.array(eevol) ;






#   outputs the energy evolution of Saxion components

def energoS(mfiles):
    eevol = []
    for f in mfiles:
        eevol.append([gm(f,'ct'),gm(f,'eKS'),gm(f,'eGS'),gm(f,'eVS'),gm(f,'eS')])
    return np.array(eevol) ;




# ------------------------------------------------------------------------------
#   load and manage sample.txt files
# ------------------------------------------------------------------------------






#   finds the sample.txt file and returns arrayS and arrayA
#   can be problematic if any is empty

def loadsample(address='./'):
    mdir = findmdir(address)
    odir = mdir[:-2]
    fina = odir+'./sample.txt'
    if os.path.exists(fina):

        with open(fina) as f:
            lines=f.readlines()
            l10 = 0
            l5 = 0
            for line in lines:
                myarray = np.fromstring(line, dtype=float, sep=' ')
                l = len(myarray)
                if l==10:
                    l10 = l10 +1
                elif l==5:
                    l5 = l5 +1
            #print('1 - lines/SAX/AX ',len(lines),l10,l5)
            #if l5 > 0 :
            arrayA = np.genfromtxt(fina,skip_header=l10)
            #    arrayS = np.empty([])
            #if l10 > 0 :
            arrayS = np.genfromtxt(fina,skip_footer=l5)
            #    arrayA = arrayS = np.empty([])
            #print('2 - lines/SAX/AX ',len(lines),len(arrayS),len(arrayA))
        #axiondata = len(arrayA) >0

#         if l10 >1 :
#             ztab1 = arrayS[:,0]
#             Thtab1 = np.arctan2(arrayS[:,4],arrayS[:,3])
#             Rhtab1 = np.sqrt(arrayS[:,3]**2 + arrayS[:,4]**2)/ztab1
#             VThtab1 = Thtab1 + (arrayS[:,3]*arrayS[:,6]-arrayS[:,4]*arrayS[:,5])/(ztab1*Rhtab1**2)
#         if axiondata:
#             ztab2 = arrayA[:,0]
#             Thtab2 = arrayA[:,2]/ztab2
#             VThtab2 = arrayA[:,3]
    # if (l10 == 0) and (l5 == 0):
    #     print('[loadsample] no output!')
    #     return ;
    # if (l10 > 0) and (l5 == 0):
    #     print('[loadsample] return only arrayS')
    #     return arrayS ;
    # if (l10 == 0) and (l5 > 0):
    #     print('[loadsample] return only arrayA')
    #     return arrayA ;
    return arrayS, arrayA;






#   Loads a sample.txt file and returns tables with data

def axev(address='./'):
    arrayS, arrayA = loadsample(address)
    lS = len(arrayS)
    lA = len(arrayA)
    ou = 0
    if lS >1 :
        ztab1 = arrayS[:,0]
        Thtab1 = np.arctan2(arrayS[:,4],arrayS[:,3])
        Rhtab1 = np.sqrt(arrayS[:,3]**2 + arrayS[:,4]**2)/ztab1
        # note that this is unshifted velocity!
        VThtab1 = Thtab1 + (arrayS[:,3]*arrayS[:,6]-arrayS[:,4]*arrayS[:,5])/(ztab1*Rhtab1**2)
        #
        strings = arrayS[:,7]
        fix = [[ztab1[0],strings[0]]]
        i = 0
        for i in range(0, len(ztab1)-1):
            if strings[i] != strings[i+1]:
                fix.append([ztab1[i+1],strings[i+1]])
        stringo = np.asarray(fix)

        ou += 1
    if len(arrayA) >0 :
        ztab2 = arrayA[:,0]
        Thtab2 = arrayA[:,2]/ztab2
        VThtab2 = arrayA[:,3]
        ou += 2

    if   ou == 3 :
        print('Saxion + Axion (ztab1, Thtab1, Rhtab1, VThtab1, stringo, ztab2, Thtab2, VThtab2)')
        return ztab1, Thtab1, Rhtab1, VThtab1, stringo, ztab2, Thtab2, VThtab2 ;
    elif ou == 1 :
        print('Saxion  (ztab1, Thtab1, Rhtab1, VThtab1, stringo)')
        return ztab1, Thtab1, Rhtab1, VThtab1, stringo ;
    elif ou == 2 :
        print('Axion (ztab2, Thtab2, VThtab2)')
        return ztab2, Thtab2, VThtab2 ;
    else :
        return ;






#   qt plot!

# def axevplot(arrayS, arrayA):
#     lS = len(arrayS)
#     lA = len(arrayA)
#
#     if l10 >1 :
#         ztab1 = arrayS[:,0]
#     #UNSHIFTED
#         Thtab1 = np.arctan2(arrayS[:,4],arrayS[:,3])
#         Rhtab1 = arrayS[:,3]**2 + arrayS[:,4]**2
#         #theta_z
#         VThtab1 = (arrayS[:,3]*arrayS[:,6]-arrayS[:,4]*arrayS[:,5])/(Rhtab1)
#         #ctheta_z
#         #VThtab1 = ztab1*(arrayS[:,3]*arrayS[:,6]-arrayS[:,4]*arrayS[:,5])/(Rhtab1) + Thtab1
#         Rhtab1 = np.sqrt(Rhtab1)/ztab1
#
#     #SHIFTED
#         arrayS[:,3] = arrayS[:,3]-ztab1*arrayS[:,9]
#         Thtab1_shift = np.arctan2(arrayS[:,4],arrayS[:,3])
#         Rhtab1_shift = arrayS[:,3]**2 + arrayS[:,4]**2
#         #theta_z
#         VThtab1_shift = (arrayS[:,3]*arrayS[:,6]-arrayS[:,4]*arrayS[:,5])/(Rhtab1_shift)
#         #ctheta_z
#         #VThtab1_shift = ztab1*(arrayS[:,3]*arrayS[:,6]-arrayS[:,4]*arrayS[:,5])/(Rhtab1_shift) + Thtab1_shift
#         Rhtab1_shift = np.sqrt(Rhtab1_shift)/ztab1
#
#     #STRINGS
#         strings = arrayS[:,7]
#         fix = [[ztab1[0],strings[0]]]
#         i = 0
#         for i in range(0, len(ztab1)-1):
#             if strings[i] != strings[i+1]:
#                 fix.append([ztab1[i+1],strings[i+1]])
#         stringo = np.asarray(fix)
#
#         co = (sizeL/sizeN)*(3/2)*(1/sizeL)**3
#     if len(arrayA) >0 :
#         ztab2 = arrayA[:,0]
#         Thtab2 = arrayA[:,2]/ztab2
#         #theta_z
#         VThtab2 = (arrayA[:,3]-Thtab2)/ztab2
#         #ctheta_z
#         #VThtab2 = arrayA[:,3]
#
#     #PLOT
#     from pyqtgraph.Qt import QtGui, QtCore
#     import pyqtgraph as pg
#
#     #QtGui.QApplication.setGraphicsSystem('raster')
#     app = QtGui.QApplication([])
#     #mw = QtGui.QMainWindow()
#     #mw.resize(800,800)
#
#     win = pg.GraphicsWindow(title="Evolution idx=0")
#     win.resize(1000,600)
#     win.setWindowTitle('jaxions evolution')
#
#     # Enable antialiasing for prettier plots
#     pg.setConfigOptions(antialias=True)
#
#     p1 = win.addPlot(title=r'theta evolution')
#
#     # p1.PlotItem.('left',r'$\theta$')
#     if l10 >1 :
#         p1.plot(ztab1,Thtab1,pen=(100,100,100))
#         p1.plot(ztab1,Thtab1_shift,pen=(255,255,255))
#     if axiondata:
#         p1.plot(ztab2,Thtab2,pen=(255,255,0))
#     p1.setLabel('left',text='theta')
#     p1.setLabel('bottom',text='time')
#
#
#     p2 = win.addPlot(title=r'theta_t evolution')
#
#     # p1.PlotItem.('left',r'$\theta$')
#     if l10 >1 :
#         p2.plot(ztab1,VThtab1,pen=(100,100,100))
#         p2.plot(ztab1,VThtab1_shift,pen=(255,255,255))
#     if axiondata:
#         p2.plot(ztab2,VThtab2,pen=(255,255,0))
#     p2.setLabel('left',text='theta_t')
#     p2.setLabel('bottom',text='time')
#
#
#     if l10 >1 :
#         win.nextRow()
#
#         p3 = win.addPlot(title=r'rho evolution')
#         p3.plot(ztab1,Rhtab1,pen=(200,0,0),name='unshifted')
#         p3.plot(ztab1,Rhtab1-arrayS[:,9],pen=(100,100,100),name='unshifted')
#         p3.plot(ztab1,Rhtab1_shift,pen=(255,255,255),name='shifted')
#
#         p3.setLabel('left',text='rho/v')
#         p3.setLabel('bottom',text='time')
#
#         p4 = win.addPlot(title=r'string evolution')
#
#         # p1.PlotItem.('left',r'$\theta$')
#
#         p4.plot(stringo[1:,0],co*stringo[1:,1]*stringo[1:,0]**2,pen=(255,255,255),symbolBrush=(153,255,204))
#         p4.setLabel('left',text='Length/Volume')
#         p4.setLabel('bottom',text='time')
#
#
#     ## Start Qt event loop unless running in interactive mode or using pyside.
#     if __name__ == '__main__':
#         import sys
#         if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
#             QtGui.QApplication.instance().exec_()
#     return ;
