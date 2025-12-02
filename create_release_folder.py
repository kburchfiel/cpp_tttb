#!/usr/bin/env python
# coding: utf-8

# # Script for creating folders that can store Linux, Windows, and Mac TTTB releases
# 
# By Ken Burchfiel
# 
# Released under the MIT License
# 
# This script makes it easier to quickly and accurately copy the files needed for a TTTB release into a folder that can then get zipped and uploaded to itch.io. You can also use it to create executable versions of the tttb.cpp and/or tttb_py_complement.py files; see argument parsing code below for more details.
# 
# (Note: Before running this code, make sure that the latest copy of your Pyinstaller-based tttb_py_complement executable, along with its '_internal' folder (if applicable), have been moved from build/dist/tttb_py_complement to build/. Disregard this note if you're using this script to build that executable, though, as in that case this step will get taken care of automatically.)
# 
# Also note that, in order for this script to successfully build the Pyinstaller executable, your base Python environment (and not just the environment from which you're calling this script) *may* need to have relatively recent versions of Numpy, Pandas, and Plotly. 

# In[1]:


import time
start_time = time.time()
import os
import shutil
import platform
import pandas as pd
import subprocess


# In[2]:


current_os = platform.system().lower() # Will be used to label certain files
# and folders
if current_os == 'darwin':
    current_os = 'osx' # Will be more intuitive for many users 
    # than 'darwin'
current_os


# In[3]:


release_folder = f'../cpp_tttb_{current_os}_release/' 
# rf = 'release folder'
release_folder


# In[4]:


# For documentation on the following code,
# see build/tttb_py_complement.ipynb.

notebook_exec = False
try:
    get_ipython()
    notebook_exec = True # Script is running within a notebook
except:
    pass

notebook_exec



# In[5]:


if notebook_exec == False:
    import argparse
    parser = argparse.ArgumentParser()
    # The items_to_compile argument specifies which source code files,
    # if any, you wish to compile into binaries. Enter 'both'
    # to compile both the tttb.cpp and tttb_py_complement files;
    # 'cpp' to compile only the tttb.cpp file; 'py' to compile
    # only the tttb_py_complement.py file; and 'neither' to 
    # skip all compilation processes (which you might want to select
    # if you've already compiled the latest versions.)

    parser.add_argument("files_to_compile", default='neither')
    args = parser.parse_args()
    files_to_compile = args.files_to_compile
else:
    files_to_compile = 'both' # neither 

# Determining, based on the argument passed to files_to_compile,
# which source code files to compile:
compile_cpp = False
compile_py = False

if files_to_compile in ['both', 'cpp']:
    compile_cpp = True
if files_to_compile in ['both', 'py']:
    compile_py = True

files_to_compile, compile_cpp, compile_py


# Specifying the names of the Python and C++ binaries:

# In[6]:


if current_os == 'windows':
    python_binary = 'tttb_py_complement.exe'
    cpp_binary = 'tttb.exe'
else:
    python_binary = 'tttb_py_complement'
    cpp_binary = 'tttb'


# In[7]:


python_binary, cpp_binary


# In[8]:


os.getcwd()


# ### Building our C++ and Python files (if requested by the caller):

# Moving into the build subfolder (simply because this is where I'm used to excuting the following commands when running them within a terminal or command prompt):

# In[9]:


os.chdir('build')


# In[10]:


os.getcwd()


# In[11]:


if compile_cpp == True:
    print("Now compiling tttb.cpp.")
    # For documentation on subprocess.run() and check_returncode(),
    # see: https://docs.python.org/3/library/subprocess.html#subprocess.run 
    compile_cpp_output_1 = subprocess.run("cmake ..", shell=True, 
                            capture_output = True)
    print(compile_cpp_output_1)
    compile_cpp_output_1.check_returncode()
    print(compile_cpp_output_1)
    compile_cpp_output_2 = subprocess.run("cmake --build .", shell=True,
                            capture_output = True)
    print(compile_cpp_output_2)
    compile_cpp_output_2.check_returncode()
    # Linux and Mac place the executable directly within the build folder;
    # meanwhile, Windows (at least with the compiler I'm using) places it
    # within a 'Debug' subfolder. Therefore, I added in the following
    # line to move this file from the 'Debug' folder to the current
    # working directory.
    if current_os == 'windows':
        shutil.copy('Debug/tttb.exe', 'tttb.exe')
else:
    print("Skipping the compilation process for tttb.cpp.")


# Running pyinstaller: 
# 
# NOTE: if you have trouble getting this code to run, make sure that you have pyinstaller added as a library within (1) the Python environment you're using to run this code and/or your base environment.
# 
# I also found that, at least within Windows, the following code took quite a while to run. I was able to get pyinstaller to build the library within my base environment in around 5-6 minutes, but after 17 minutes, this cell still hadn't finished. Therefore, you might want to take care of the following steps yourself, then select 'cpp' or 'neither' as your argument for this code rather than 'both' or 'py.'
# 
# (The code took around 40 seconds to execute within Linux, on the other hand.)

# In[12]:


if compile_py == True:
    print("Now compiling tttb_py_complement.py.")
    # The -y argument in the following call will allow any existing
    # material within the output folder to get deleted and replaced.

    if current_os == 'osx': # For the OSX release, I'll store the entire
        # Pyinstaller package as a single file via the --onefile argument. 
        # This makes for a slower load time
        # than the default method (which creates both a main Python binary
        # and an _internal folder for library-related files), but it
        # dramatically reduces the amount of programs that the user
        # will need to manually authorize.
        # See https://pyinstaller.org/en/stable/usage.html for details
        # on both the --onefile argument and the --noconfirm argument
        # (which eliminates the need to authorize Pyinstaller's
        # overwriting dist folder files.)

        pyinstaller_subprocess_command = "pyinstaller \
tttb_py_complement.py --noconfirm --onefile"
    else:
        pyinstaller_subprocess_command = "pyinstaller \
tttb_py_complement.py --noconfirm"
    compile_py_output = subprocess.run(
        pyinstaller_subprocess_command, shell = True, 
        capture_output = True, check = True)
    print(compile_py_output)
    # Copying tttb_py_complement binary and its corresponding
    # _internal folder into the build folder (where the C++ program expects
    # to find it):
    if current_os == 'osx':
        shutil.copy(
            f'dist/{python_binary}', 
            f'{python_binary}')
    else: # No _internal folder will be created for OSX
        # releases, and the executable folder path will be different as well.
        if '_internal' in os.listdir():
            shutil.rmtree('_internal')
        # Based on datainsight's StackOverflow
            # answer at https://stackoverflow.com/a/70075600/13097194 
        shutil.copytree(
            'dist/tttb_py_complement/_internal', '_internal')
        # Note that shutil.copy() will overwrite the existing copy of the file
        # (if any) in the destination folder with the new copy; see
        # https://docs.python.org/3/library/shutil.html#shutil.copy
        # for details
        shutil.copy(
            f'dist/tttb_py_complement/{python_binary}', 
            f'{python_binary}')
else:
    print("Skipping the compilation process for tttb_py_complement.py.")


# Moving back into our main folder:

# In[13]:


os.chdir('..')
os.getcwd()


# In[14]:


if release_folder.replace('../', '').replace('/', '') in os.listdir('../'):
    # NOTE: this will remove the existing folder (which will allow us to
    # more easily recreate a clean copy).
    shutil.rmtree(release_folder)
os.mkdir(release_folder)

# Creating relevant directories:

os.mkdir(release_folder + 'Files/')
os.mkdir(release_folder + 'build/')
os.mkdir(release_folder + 'Files/Multiplayer/')
os.mkdir(release_folder + 'Files/MP_Test_Result_Files_To_Combine/')
os.mkdir(release_folder + 'Files/MP_Word_Result_Files_To_Combine/')
os.mkdir(release_folder + 'Visualizations/')
os.mkdir(release_folder + 'Visualizations/Multiplayer')
os.mkdir(release_folder + 'Visualizations/Single_Player')
os.mkdir(release_folder + 'Analyses/')
os.mkdir(release_folder + 'Analyses/Multiplayer')
os.mkdir(release_folder + 'Analyses/Single_Player')


# Making sure that the headers in our 'headers only' file match the ones I've been using in my own gameplay sessions: (sometimes I only update the latter rather than the former.)

# Copying new versions of our word result, test result, and Bible files over to the Files folder:

# In[15]:


if list(pd.read_csv(
'Files/word_results_headers_only.csv').columns) != list(
pd.read_csv('Files/word_results.csv').columns):
    raise ValueError ("Columns don't match!")


# In[16]:


if list(pd.read_csv(
    'Files/test_results_headers_only.csv').columns) != list(
pd.read_csv('Files/test_results.csv').columns):
    raise ValueError ("Columns don't match!")


# In[17]:


if list(pd.read_csv(
    'Files/game_config_headers_only.csv').columns) != list(
pd.read_csv('Files/game_config.csv').columns):
    raise ValueError ("Columns don't match!")


# In[18]:


shutil.copy('Files/word_results_headers_only.csv',
            release_folder+'Files/word_results.csv')
shutil.copy('Files/game_config_headers_only.csv',
            release_folder+'Files/game_config.csv')
shutil.copy('Files/test_results_headers_only.csv',
            release_folder+'Files/test_results.csv')
shutil.copy('Catholic_Public_Domain_Bible/CPDB_for_TTTB.csv',
            release_folder+'Files/CPDB_for_TTTB.csv')
shutil.copy('README_download_link.txt',release_folder + 'README_download_link.txt')


# Copying TTTB C++ and Python binaries, along with supporting files for Python binary, to the release folder:

# In[19]:


shutil.copy(f'build/{cpp_binary}',
            release_folder+f'build/{cpp_binary}')
shutil.copy(f'build/{python_binary}',
            release_folder+f'build/{python_binary}')
# For details on shutil.copytree, see 
# https://docs.python.org/3/library/shutil.html
if current_os != 'osx':
    shutil.copytree('build/_internal', release_folder+'build/_internal')


# Copying Markdown and PDF versions of Readme over to release folder:
# 
# (I commented out this option because I decided to share the Readme as a separate file instead. That way, I wouldn't have to create new zip files of release folders, each 75+ megabytes in size, for distribution on itch.io just because I updated the game's instructions.)

# In[20]:


# shutil.copy('README.md',
#             release_folder+'README.md')
# shutil.copy('README.pdf',
#             release_folder+'README.pdf')


# In[21]:


end_time = time.time()
print(f"Finished running the create_release_folder script \
in {round(end_time - start_time, 3)} seconds. A \
clean copy of TTTB can now be found at:\n{release_folder}")

