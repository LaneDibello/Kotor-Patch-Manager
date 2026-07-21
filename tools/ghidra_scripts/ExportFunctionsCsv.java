// Exports named functions to the CSV consumed by `SqliteTools import-ghidra`.
//
// Functions are named flat as Class__Function in Ghidra's Global namespace, so
// each name is split on the LAST "__" into (class_name, function_name); names
// with no "__" (free functions) get class_name = "Global", matching the
// address-DB convention. Names are Itanium-spelled, so a few differ from the
// GameAPI aliases the DB uses (HasSpell vs HasSpellPrereq) and need re-aliasing.
//
// @category KOTOR
// @menupath Tools.KOTOR.Export Functions CSV

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.symbol.SourceType;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;

public class ExportFunctionsCsv extends GhidraScript {

    @Override
    public void run() throws Exception {
        File out = askFile("Export functions CSV", "Save");

        int exported = 0;
        try (PrintWriter w = new PrintWriter(new FileWriter(out))) {
            w.println("class_name,function_name,address,calling_convention,param_size_bytes,notes");

            FunctionManager fm = currentProgram.getFunctionManager();
            for (Function fn : fm.getFunctions(true)) {
                // Skip auto-named placeholders; only export intentionally-named functions.
                if (fn.getSymbol().getSource() == SourceType.DEFAULT) {
                    continue;
                }
                String name = fn.getName();
                if (name.startsWith("FUN_") || name.startsWith("thunk_")) {
                    continue;
                }

                String className;
                String functionName;
                int sep = name.lastIndexOf("__");
                if (sep < 0) {
                    className = "Global";
                    functionName = name;
                } else {
                    className = name.substring(0, sep);
                    functionName = name.substring(sep + 2);
                }

                String cc = fn.getCallingConventionName();
                if (cc == null || cc.equals("unknown") || cc.equals("default")) {
                    cc = "";
                }

                // Caller-visible argument bytes: each parameter occupies a
                // 4-byte-aligned stack slot under the i386 System V ABI.
                int paramBytes = 0;
                boolean hasParams = false;
                for (Parameter p : fn.getParameters()) {
                    int len = p.getDataType().getLength();
                    if (len < 1) {
                        len = 4;
                    }
                    paramBytes += ((len + 3) / 4) * 4;
                    hasParams = true;
                }
                String paramBytesStr = hasParams ? String.valueOf(paramBytes) : "";

                w.println(csv(className) + "," + csv(functionName) + ","
                        + String.format("0x%08x", fn.getEntryPoint().getOffset()) + ","
                        + cc + "," + paramBytesStr + "," + csv(fn.getComment()));
                exported++;
            }
        }

        println("Exported " + exported + " functions to " + out.getAbsolutePath());
    }

    // Quote fields containing a comma, quote, or newline (RFC-4180).
    private String csv(String s) {
        if (s == null) {
            return "";
        }
        if (s.indexOf(',') >= 0 || s.indexOf('"') >= 0 || s.indexOf('\n') >= 0) {
            return "\"" + s.replace("\"", "\"\"") + "\"";
        }
        return s;
    }
}
