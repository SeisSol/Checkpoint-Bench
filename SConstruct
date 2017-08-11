#! /usr/bin/python
##
# @file
# This file is part of SeisSol.
#
# @author Sebastian Rettenberger (sebastian.rettenberger AT tum.de, http://www5.in.tum.de/wiki/index.php/Sebastian_Rettenberger)
#
# @section LICENSE
# Copyright (c) 2016, SeisSol Group
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# @section DESCRIPTION
#

# operation system (required for exectuion environment)
import os
import sys

# import helpers
import arch

# print the welcome message
print '*******************************************************'
print '** Welcome to the build script of SeisSol-Checkpoint **'
print '*******************************************************'
print 'Copyright (c) 2016, SeisSol Group'

# Check if we the user wants to show help only
if '-h' in sys.argv or '--help' in sys.argv:
  helpMode = True
else:
  helpMode = False
  
def ConfigurationError(msg):
    """Print the error message and exit. Continue only
    if the user wants to show the help message"""
    
    if not helpMode:
        print msg
        Exit(1) 

#
# set possible variables
#
vars = Variables()

# read parameters from a file if given
vars.AddVariables(
  PathVariable( 'buildVariablesFile', 'location of the python file, which contains the build variables', None, PathVariable.PathIsFile )
)
env = Environment(variables=vars)
if 'buildVariablesFile' in env:
  vars = Variables(env['buildVariablesFile'])

# SeisSol specific variables
vars.AddVariables(
EnumVariable( 'equations',
                'system of PDEs that will be solved',
                'elastic',
                allowed_values=('elastic',)
              ),
                  
  EnumVariable( 'order',
                'convergence order of the ADER-DG method',
                '6',
                allowed_values=('2', '3', '4', '5', '6', '7', '8')
              ),
                  
  PathVariable( 'buildDir', 'where to build the code', 'build', PathVariable.PathIsDirCreate ),
  
  EnumVariable( 'compileMode', 'mode of the compilation', 'release',
                allowed_values=('debug', 'release', 'relWithDebInfo')
              ),

  EnumVariable( 'parallelization', 'level of parallelization', 'none',
                allowed_values=('none', 'omp', 'mpi', 'hybrid')
              ),
  
  BoolVariable( 'hdf5', 'use hdf5 library for data output', False ),
  
  BoolVariable( 'sionlib', 'use sion library for checkpointing', False ),

  EnumVariable( 'logLevel',
                'logging level. \'debug\' runs assertations and prints all information available, \'info\' prints information at runtime (time step, plot number), \'warning\' prints warnings during runtime, \'error\' is most basic and prints errors only',
                'info',
                allowed_values=('debug', 'info', 'warning', 'error')
              ),
)

# external variables
vars.AddVariables(
  PathVariable( 'hdf5Dir',
                'HDF5 installation directory',
                None,
                PathVariable.PathAccept ),
                  
  PathVariable( 'zlibDir',
                'zlib installation directory',
                None,
                PathVariable.PathAccept ),

  PathVariable( 'sionlibDir',
                'sionlib installation directory',
                None,
                PathVariable.PathAccept ),
                  
  EnumVariable( 'compiler',
                'Select the compiler (default: intel)',
                'intel',
                allowed_values=('intel', 'gcc', 'cray_intel', 'cray_gcc')),
                
  BoolVariable( 'useExecutionEnvironment',
                'set variables set in the execution environment',
                True ),

  EnumVariable( 'arch',
                'precision -- s for single- and d for double precision -- and architecture used. Warning: \'noarch\' calls the fall-back code and is outperformed by architecture-specific optimizations (if available) greatly.',
                'dnoarch',
                allowed_values=arch.getArchitectures()
              ),

  EnumVariable( 'scalasca', 'instruments code with scalasca. \n \'default\': instruments only outer loops. \n',
                'none',
                allowed_values=('none', 'default', 'kernels')
              ),
)

# set environment
env = Environment(variables=vars)

if env['useExecutionEnvironment']:
    env['ENV'] = os.environ

# generate help text
Help(vars.GenerateHelpText(env))

# handle unknown, maybe misspelled variables
unknownVariables = vars.UnknownVariables()

# remove the buildVariablesFile from the list of unknown variables (used before)
if 'buildVariablesFile' in unknownVariables:
  unknownVariables.pop('buildVariablesFile')

# exit in the case of unknown variables
if unknownVariables:
  ConfigurationError("*** The following build variables are unknown: " + str(unknownVariables.keys()))
  
# check for architecture
if env['arch'] == 'snoarch' or env['arch'] == 'dnoarch':
  print "*** Warning: Using fallback code for unknown architecture. Performance will suffer greatly if used by mistake and an architecture-specific implementation is available."

#
# preprocessor, compiler and linker
#

numberOfQuantities = {
  'elastic' : 9
}

# Basic compiler setting
if env['compiler'] == 'intel':
    env['CC'] = 'icc'
    env['CXX'] = 'icpc'
    env['F90'] = 'ifort'
elif env['compiler'] == 'gcc':
    env['CC'] = 'gcc'
    env['CXX'] = 'g++'
    env['F90'] = 'gfortran'
elif env['compiler'].startswith('cray_'):
    env['CC'] = 'cc'
    env['CXX'] = 'CC'
    env['F90'] = 'ftn'
else:
    assert(false)
    
# Parallel compiler required?
if env['parallelization'] in ['mpi', 'hybrid']:
    env.Tool('MPITool')
    
    # Do not include C++ MPI Bindings
    env.Append(CPPDEFINES=['OMPI_SKIP_MPICXX'])

# Remove any special compiler configuration
# Do this after the MPI tool is called, because the MPI Tool checks for special compilers
if env['compiler'].startswith('cray'):
    #env.Append(LINKFLAGS=['-dynamic'])
    env['compiler'] = env['compiler'].replace('cray_', '')

# Include preprocessor in all Fortran builds
env['F90COM'] = env['F90PPCOM']

# Use Fortran for linking
env['LINK'] = env['F90']

#
# Scalasca
#

# instrument code with scalasca
if env['scalasca'] in ['default', 'kernels']:
  for mode in ['CC', 'CXX', 'F90', 'LINK']:
    l_scalascaPrelude = 'scalasca -instrument -comp=none -user '

    if env['parallelization'] in ['mpi', 'hybrid']:
      l_scalascaPrelude = l_scalascaPrelude + '-mode=MPI '

    env[mode] = l_scalascaPrelude + env[mode]

if env['scalasca'] in ['default_2.x', 'kernels_2.x']:
  l_scorepArguments = " --noonline-access --nocompiler --user "
  if env['parallelization'] == 'none':
    l_scorepArguments = l_scorepArguments + ' --mpp=none '
  if env['parallelization'] in ['mpi', 'hybrid']:
    l_scorepArguments = l_scorepArguments + ' --mpp=mpi '

  if env['parallelization'] in ['mpi', 'none']:
    l_scorepCxxArguments = l_scorepArguments + ' --thread=none '
  else:
    l_scorepCxxArguments = l_scorepArguments + ' --thread=omp '

  for mode in ['F90']:
    env[mode] = 'scorep' + l_scorepArguments + ' --thread=none ' + env[mode]
  for mode in ['CC', 'CXX', 'LINK']:
    env[mode] = 'scorep' + l_scorepCxxArguments + env[mode]
    
# kernel instrumentation with scalasca
if env['scalasca'] == 'kernels_2.x':
  env.Append(CPPDEFINES=['INSTRUMENT_KERNELS'])
    
#
# Common settings
#

# enforce restrictive C/C++-Code
env.Append(CFLAGS   = ['-Wall', '-Werror', '-ansi'],
           CXXFLAGS = ['-Wall', '-Werror', '-ansi'])
if env['compiler'] == 'intel':
    env.Append(CXXFLGAS = ['-wd13379'])

# run preprocessor before compiling
env.Append(F90FLAGS=['-cpp'])

# Use complete line
if env['compiler'] == 'gcc':
    env.Append(F90FLAGS=['-ffree-line-length-none'])
  
# enforce 8 byte precision for reals (required in SeisSol) and compile time boundary check
if env['compiler'] == 'intel':
    env.Append(F90FLAGS=['-r8', '-WB'])
elif env['compiler'] == 'gcc':
    env.Append(F90FLAGS=['-fdefault-real-8'])
    
# Align structs and arrays
if env['compiler'] == 'intel':
    # TODO Check if Fortran alignment is still necessary in the latest version 
    env.Append(F90LFAGS=['-align', '-align', 'array64byte'])

# Add  Linker-flags  for cross-compiling
if env['compiler'] == 'intel':
    env.Append(LINKFLAGS=['-nofor-main', '-cxxlib']) #Add -ldmalloc for ddt
elif env['compiler'] == 'gcc':
    env.Append(LIBS=['stdc++'])
    
#
# Architecture dependent settings
#
archFlags = arch.getFlags(env['arch'], env['compiler'])
env.Append( CFLAGS    = archFlags,
            CXXFLAGS  = archFlags,
            F90FLAGS  = archFlags,
            LINKFLAGS = archFlags )
env.Append(CPPDEFINES=['ALIGNMENT=' + str(arch.getAlignment(env['arch'])), env['arch'].upper()])
  
#
# Compile mode settings
#

# set (pre-)compiler flags for the compile modes
if env['compileMode'] == 'debug':
  env.Append(F90FLAGS = ['-O0'],
             CLFGAGS  = ['-O0'],
             CXXFLAGS = ['-O0'])
  if env['compiler'] == 'intel':
      env.Append(F90FLAGS = ['-shared-intel', '-check'],
                 CLFGAGS  = ['-shared-intel'],
                 CXXFLAGS = ['-shared-intel'])
  else:
      env.Append(F90FLAGS = ['-fcheck=all'])
if env['compileMode'] in ['debug', 'relWithDebInfo']:
  env.Append(F90FLAGS  = ['-g'],
             CFLAGS    = ['-g'],
             CXXFLAGS  = ['-g'],
             LINKFLAGS = ['-g', '-rdynamic'])
  if env['compiler'] == 'intel':
      env.Append(F90FLAGS  = ['-traceback'],
                 CFLAGS    = ['-traceback'],
                 CXXFLAGS  = ['-traceback'])
  else:
      env.Append(F90FLAGS  = ['-fbacktrace'])

if env['compileMode'] in ['relWithDebInfo', 'release']:
    env.Append(CPPDEFINES = ['NDEBUG'])
    env.Append(F90FLAGS = ['-O2'],
               CFLAGS   = ['-O2'], 
               CXXFLAGS = ['-O2'])
    if env['compiler'] == 'intel':
        env.Append(F90FLAGS = ['-fno-alias'])
    
#
# Basic preprocessor defines
#

# set precompiler mode for the number of quantities and basis functions
env.Append(CPPDEFINES=['CONVERGENCE_ORDER='+env['order']])
env.Append(CPPDEFINES=['NUMBER_OF_QUANTITIES=' + str(numberOfQuantities[ env['equations'] ])])

# add parallel flag for mpi
if env['parallelization'] in ['mpi', 'hybrid']:
    # TODO rename PARALLEL to USE_MPI in the code
    env.Append(CPPDEFINES=['USE_MPI', 'PARALLEL'])

# add OpenMP flags
if env['parallelization'] in ['omp', 'hybrid']:
    env.Append(CPPDEFINES=['OMP'])
    env.Append(CFLAGS    = ['-fopenmp'],
               CXXFLAGS  = ['-fopenmp'],
               F90FLAGS  = ['-fopenmp'],
               LINKFLAGS = ['-fopenmp'])
  
# set level of logger for rank 0 and C++
if env['logLevel'] == 'debug':
  env.Append(CPPDEFINES=['LOG_LEVEL=3'])
elif env['logLevel'] == 'info':
  env.Append(CPPDEFINES=['LOG_LEVEL=2'])
elif env['logLevel'] == 'warning':
  env.Append(CPPDEFINES=['LOG_LEVEL=1'])
elif env['logLevel'] == 'error':
  env.Append(CPPDEFINES=['LOG_LEVEL=0'])
else:
  assert(false)

# Enable checkpoint bench code
env.Append(CPPDEFINES=['CHECKPOINT_BENCH'])

# add include path for submodules
env.Append( CPPPATH=['#/submodules', '#/submodules/glm', '#/submodules/async'] )

#
# add libraries
#

# Library pathes
env.Tool('DirTool', fortran=True)

# GLM
# Workaround for wrong C++11 detection
env.Append(CPPDEFINES=['GLM_FORCE_COMPILER_UNKNOWN'])

# HDF5
if env['hdf5']:
    env.Tool('Hdf5Tool', required=(not helpMode))
    env.Append(CPPDEFINES=['USE_HDF'])
    
# sionlib still need to create a Tool for autoconfiguration
if env['sionlib']:
  env.Tool('SionTool', parallel=(env['parallelization'] in ['hybrid', 'mpi']))
else:
  env['sionlib'] = False

# add pathname to the list of directories wich are search for include
env.Append(CPPPATH=['#/src', '#/seissol/src',
                     '#/seissol/src/Equations/' + env['equations'],
                     '#/seissol/src/Equations/' + env['equations'] + '/generated_code'])

#
# setup the program name and the build directory
#
env['programFile'] = '%s/checkpoint' % (env['buildDir'],)

# build directory
env['buildDir'] = '%s/build' %(env['buildDir'],)

# get the source files
env.sourceFiles = []
env.generatedTestSourceFiles = []

Export('env')
SConscript('src/SConscript', variant_dir='#/'+env['buildDir'], src_dir='#/', duplicate=0)
SConscript('seissol/src/Checkpoint/SConscript', variant_dir='#/'+env['buildDir']+'/Checkpoint', duplicate=0)
SConscript('seissol/src/Parallel/SConscript', variant_dir='#/'+env['buildDir']+'/Parallel', duplicate=0)
Import('env')

# remove .mod entries for the linker
sourceFiles = []
for sourceFile in env.sourceFiles:
  sourceFiles.append(sourceFile[0])

#print env.Dump()

# build standard version
env.Program('#/'+env['programFile'], sourceFiles)
