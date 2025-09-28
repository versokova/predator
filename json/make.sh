#!/bin/bash

BUILDDIR=../json_build
ATDDIR=cl-atd

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	SED="sed -r"
else # "darwin"* | "freebsd"*
	SED="sed -E"
fi

function atdgen_bug () {
	atd=$1
	$SED 's/_(j|t)\./\./g' $atd > tmp
	cp tmp $atd
	rm -f tmp
}


atdgen -json "$ATDDIR"/Loc.atd -o "$BUILDDIR"/Loc
atdgen -json "$ATDDIR"/Type.atd -o "$BUILDDIR"/Type
atdgen -json "$ATDDIR"/Operand.atd -o "$BUILDDIR"/Operand
atdgen -json "$ATDDIR"/Fnc.atd -o "$BUILDDIR"/Fnc
atdgen -json "$ATDDIR"/Var.atd -o "$BUILDDIR"/Var
atdgen -json "$ATDDIR"/Storage.atd -o "$BUILDDIR"/Storage

(
cd "$BUILDDIR"

atdgen_bug Type.mli
atdgen_bug Type.ml
atdgen_bug Operand.mli
atdgen_bug Operand.ml
atdgen_bug Fnc.mli
atdgen_bug Fnc.ml
atdgen_bug Var.mli
atdgen_bug Var.ml
atdgen_bug Storage.mli
atdgen_bug Storage.ml

ocamlfind ocamlc -c Loc.mli -package atdgen-runtime
ocamlfind ocamlc -c Loc.ml -package atdgen-runtime
ocamlfind ocamlc -c Type.mli -package atdgen-runtime
ocamlfind ocamlc -c Type.ml -package atdgen-runtime
ocamlfind ocamlc -c Operand.mli -package atdgen-runtime
ocamlfind ocamlc -c Operand.ml -package atdgen-runtime
ocamlfind ocamlc -c Fnc.mli -package atdgen-runtime
ocamlfind ocamlc -c Fnc.ml -package atdgen-runtime
ocamlfind ocamlc -c Var.mli -package atdgen-runtime
ocamlfind ocamlc -c Var.ml -package atdgen-runtime
ocamlfind ocamlc -c Storage.mli -package atdgen-runtime
ocamlfind ocamlc -c Storage.ml -package atdgen-runtime

ocamlfind ocamlc -c ../json/Check.ml -package atdgen-runtime -o "$BUILDDIR"/Check
ocamlfind ocamlc -o check Loc.cmo Type.cmo Operand.cmo Fnc.cmo Var.cmo Storage.cmo Check.cmo  -package atdgen-runtime  -linkpkg

)

# State.cmo --> Contract.cmo

exit
#ocamlfind ocamlopt -c test.ml -package atdgen-runtime
#ocamlfind ocamlopt -o test Loc.cmx Type.cmx Operand.cmx Fnc.cmx Var.cmx Storage.cmx test.cmx  -package atdgen-runtime  -linkpkg
ocamlfind ocamlopt -c abduce.ml -package atdgen-runtime
ocamlfind ocamlopt -o abduce Loc.cmx Type.cmx Operand.cmx Fnc.cmx Var.cmx Storage.cmx abduce.cmx  -package atdgen-runtime  -linkpkg
ocamlc -c Loc.cmx Type.cmx Operand.cmx Fnc.cmx Var.cmx Storage.cmx abduce.cmx  -package atdgen-runtime  -linkpkg
exit
#./test < ../tests/fnc.json
ydump

./check < ../tests/sll1.json
utop -init utop.ocamlinit

