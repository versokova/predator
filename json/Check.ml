module AGU = Atdgen_runtime.Util
(* Ag_util for older lib <2.0 *)

(*
libppx-visitors-ocaml-dev
ppx_fields_conv               v0.11.0  Generation of accessor and iteration functions for ocaml records
ppx_variants_conv             v0.11.0  Generation of accessor and iteration functions for ocaml variant types
*)

(* read Storage from stdin *)
let c = AGU.Json.from_channel Storage.read_t stdin

(* * * * * * * * * * * * * * * main * * * * * * * * * * * * * * *)
let () =
    (* write Storage to stdout *)
    AGU.Json.to_channel Storage.write_t stdout c;
    print_newline ();
