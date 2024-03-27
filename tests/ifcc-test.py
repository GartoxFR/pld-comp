#!/usr/bin/env python3

# This scripts runs GCC as well as IFCC on each test-case provided and compares the results.
#
# input: the test-cases are specified either as individual
#         command-line arguments, or as part of a directory tree
#
# output:
#
# The script is divided in three distinct steps:
# - in the ARGPARSE step, we understand the command-line arguments
# - in the PREPARE step, we copy all our test-cases into a single directory tree
# - in the TEST step, we actually run GCC and IFCC on each test-case
#
#

import argparse
import os
import shutil
import sys
import asyncio
from pathlib import Path
import filecmp

######################################################################################
# ARGPARSE step: make sense of our command-line arguments

argparser = argparse.ArgumentParser(
    description="Compile multiple programs with both GCC and IFCC, run them, and compare the results.",
    epilog=""
)

argparser.add_argument('input', metavar='PATH', nargs='+',
                       help='For each path given: if it\'s a file, use this file;'
                       + ' if it\'s a directory, use all *.c files in this subtree')

argparser.add_argument('-d', '--debug', action="count", default=0,
                       help='Increase quantity of debugging messages (only useful to debug the test script itself)')
argparser.add_argument('-v', '--verbose', action="count", default=0,
                       help='Increase verbosity level. You can use this option multiple times.')

args = argparser.parse_args()

if args.debug >= 2:
    print('debug: command-line arguments ' + str(args))

if Path.cwd().name == "ifcc-test-output":
    print('error: cannot run from within the output directory')
    sys.exit(1)

out_path = Path("ifcc-test-output")
if out_path.exists():
    # cleanup previous output directory
    shutil.rmtree(out_path)
out_path.mkdir()

# Then we process the inputs arguments i.e. filenames or subtrees
input_files = []
for path_str in args.input:
    path = Path(path_str)
    if path.is_file():
        if path.suffix == '.c':
            input_files.append(path)
        else:
            print(f"error: incorrect filename suffix (should be '.c'): {path}")
            exit(1)
    elif path.is_dir() and path.name != "ifcc-test-output":
        for dirpath, _, filenames in os.walk(path):
            for filename in filenames:
                path = Path(dirpath) / filename
                if path.suffix == '.c':
                    input_files.append(path)
    else:
        print(f"error: cannot read input path: {path}")
        sys.exit(1)

# debug: after treewalk
if args.debug:
    print("debug: list of files after tree walk: ", " ".join(map(str, input_files)))

# sanity check
if len(input_files) == 0:
    print("error: found no test-case in: ", " ".join(args.input))
    sys.exit(1)


######################################################################################
# TEST step: actually compile all test-cases with both compilers


def test_result(ok: bool, job_path: Path, message: str = None):
    if ok:
        print('.', end='', flush=True)
    else:
        print()
        print(f"TEST-CASE: {job_path}")
        print(f"TEST FAIL { f' ({message})' if message else ''}")
    return ok


async def job(input_file: Path, semaphore: asyncio.Semaphore):

    async with semaphore:

        p = out_path / f"{input_file.parent.parent.name}-{input_file.parent.name}-{input_file.name}"
        p.mkdir()
        shutil.copyfile(input_file, p / "input.c")

        # Reference compiler = GCC
        gcc_out = p / "compile-gcc.txt"
        ifcc_out = p / "compile-ifcc.txt"
        gcc_comp = await asyncio.create_subprocess_exec(
            "gcc", "-S", "-o",
            p / "asm-gcc.s",
            p / "input.c",
            stderr=gcc_out.open("w")
        )
        # IFCC compiler
        ifcc_comp = await asyncio.create_subprocess_exec(
            Path.cwd() /"compiler/ifcc", p / "input.c",
            "-fPIE",
            stdout=(p / "asm-ifcc.s").open("w"),
            stderr=ifcc_out.open("w")
        )
        await gcc_comp.wait()
        await ifcc_comp.wait()
        with gcc_out.open('a') as file:
            file.write(f"return code: {gcc_comp.returncode}")
        with ifcc_out.open('a') as file:
            file.write(f"return code: {ifcc_comp.returncode}")

        # ifcc correctly rejects invalid program -> test-case ok
        if gcc_comp.returncode != 0 and ifcc_comp.returncode != 0:
            return test_result(True, p, "both failed")

        # ifcc wrongly accepts invalid program -> error
        if gcc_comp.returncode != 0 and ifcc_comp.returncode == 0:
            return test_result(False, p, "your compiler accepts an invalid program")

        # ifcc wrongly rejects valid program -> error
        if gcc_comp.returncode == 0 and ifcc_comp.returncode != 0:
            ok = test_result(False, p, "your compiler rejects a valid program")
            if args.verbose:
                print((p / "ifcc-compile.txt").read_text())
            return ok

        # ifcc accepts to compile valid program -> let's link it
        gcc_out = p / "link-gcc.txt"
        ifcc_out = p / "link-ifcc.txt"
        gcc_link = await asyncio.create_subprocess_exec(
            "gcc", "-o",
            p / "exe-gcc",
            p / "asm-gcc.s",
            stderr=gcc_out.open("w")
        )
        ifcc_link = await asyncio.create_subprocess_exec(
            "gcc", "-o",
            p / "exe-ifcc",
            p / "asm-ifcc.s",
            "-fPIE",
            stderr=ifcc_out.open("w")
        )
        await gcc_link.wait()
        await ifcc_link.wait()
        with gcc_out.open('a') as file:
            file.write(f"return code: {gcc_link.returncode}")
        with ifcc_out.open('a') as file:
            file.write(f"return code: {ifcc_link.returncode}")

        if ifcc_link.returncode:
            ok = test_result(False, p, "your compiler produces incorrect assembly")
            if args.verbose:
                print((p / "ifcc-link.txt").read_text())
            return ok

        # both compilers  did produce an  executable, so now we  run both
        # these executables and compare the results.
        gcc_out = p / "execute-gcc.txt"
        ifcc_out = p / "execute-ifcc.txt"
        gcc_exec = await asyncio.create_subprocess_exec(
            p / "exe-gcc",
            stdout=gcc_out.open("w")
        )
        ifcc_exec = await asyncio.create_subprocess_exec(
            p / "exe-ifcc",
            stdout=ifcc_out.open("w")
        )
        await gcc_exec.wait()
        await ifcc_exec.wait()
        with gcc_out.open('a') as file:
            file.write(f"return code: {gcc_exec.returncode}")
        with ifcc_out.open('a') as file:
            file.write(f"return code: {ifcc_exec.returncode}")

        if gcc_exec.returncode != ifcc_exec.returncode:
            return test_result(False, p, "different return codes at execution")

        if not filecmp.cmp(gcc_out, ifcc_out):
            ok = test_result(False, p, "different stdout at execution")
            if args.verbose:
                print("GCC:")
                print(gcc_out.read_text())
                print("IFCC:")
                print(ifcc_out.read_text())
            return ok

        # last but not least
        return test_result(True, p)


async def main():
    semaphore = asyncio.Semaphore(20)
    res = await asyncio.gather(*(job(file, semaphore) for file in input_files))
    return res.count(False)

failed = asyncio.run(main())

print()
if failed:
    print(f"{failed} test{'s' if failed > 1 else ''} failed")
else:
    print("All tests passed")
