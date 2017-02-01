#!/usr/bin/env python

# by: Kwabena W. Agyeman - kwagyeman@openmv.io

import argparse, fnmatch, os, sys

def try_which(program):
    if os.path.dirname(program):
        if os.path.isfile(program) and os.access(program, os.X_OK):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            file_exe = os.path.join(path.strip('\'').strip('\"'), program)
            if os.path.isfile(file_exe) and os.access(file_exe, os.X_OK):
                return file_exe

def which(program):
    exes = []
    if sys.platform.startswith("win") and not program.lower().endswith(".exe"):
        exes.extend(["exe", "exE", "eXe", "eXE", "Exe", "ExE", "EXe", "EXE"])
    if not exes: return try_which(program)
    for exe in exes:
        if try_which(program + '.' + exe): return program
    return None
kSignCMDAvailable = which("kSignCMD")
codsignAvailable = which("codesign")

def getPFXFile():
    file = os.path.join(os.path.expanduser('~'), "certificate.pfx")
    return None if not os.path.isfile(file) else file
PFXFile = getPFXFile()

def getPFXPass():
    file = os.path.join(os.path.expanduser('~'), "certificate.txt")
    if not os.path.isfile(file): return None
    with open(file, 'r') as file: return file.readline().strip()
PFXPass = getPFXPass()

def getIdentity():
    file = os.path.join(os.path.expanduser('~'), "identity.txt")
    if not os.path.isfile(file): return None
    with open(file, 'r') as file: return file.readline().strip()
identity = getIdentity()

def signFile(file):
    if sys.platform.startswith("win"):
        if kSignCMDAvailable and PFXFile and PFXPass:
            if not os.system("kSignCMD" + \
            " /f " + PFXFile.replace("/", "\\") + \
            " /p " + PFXPass + \
            " " + file.replace("/", "\\")):
                print "Success"
            else:
                print "Failure"
                raise
        else: print "Skipping"
        return
    elif sys.platform == "darwin":
        if codsignAvailable and identity:
            if not os.system("codesign" + \
            " -s " + identity + " " + file):
                print "Success"
            else:
                print "Failure"
                raise
        else: print "Skipping"
        return
    print "Success"

def try_signFile(file):
    print "Signing %s..." % file,
    try: signFile(file)
    except:
        print "Trying again...",
        try: signFile(file)
        except:
            print("Failed to sign %s." % file)
            # Don't die...
            pass

def main():
    __folder__ = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description = "Sign Script")
    parser.add_argument("target", help = "File or Directory")
    args = parser.parse_args()

    target = args.target
    if target.endswith('*'):
        target = target[:-1]
    if not os.path.isabs(target):
        target = os.path.abspath(target)

    if os.path.isfile(target): try_signFile(target)
    else:

        extensions = ["*.[rR][uU][nN]", "*.[sS][oO]"]
        if sys.platform.startswith("win"):
            extensions = ["*.[eE][xX][eE]", "*.[dD][lL][lL]"]
        elif sys.platform == "darwin":
            extensions = ["*.[aA][pP][pP]", "*.[dD][yY][lL][iI][bB]"]

        for dirpath, dirnames, filenames in os.walk(target):
            paths = dirnames + filenames
            for extension in extensions:
                for path in fnmatch.filter(paths, extension):
                    try_signFile(os.path.join(dirpath, path))

if __name__ == "__main__":
    main()
