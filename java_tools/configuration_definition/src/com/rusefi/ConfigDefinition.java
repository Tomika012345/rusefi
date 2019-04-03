package com.rusefi;

import java.io.*;
import java.util.Date;

/**
 * (c) Andrey Belomutskiy
 * 1/12/15
 */
@SuppressWarnings("StringConcatenationInsideStringBufferAppend")
public class ConfigDefinition {
    public static final String EOL = "\n";
    private static final String INPUT_FILE_NAME = "rusefi_config.txt";
    static final String MESSAGE = "was generated automatically by ConfigDefinition.jar based on " + INPUT_FILE_NAME + " " + new Date();
    static final String TS_FILE_INPUT_NAME = "rusefi.input";
    private static final String STRUCT_NO_PREFIX = "struct_no_prefix ";
    private static final String STRUCT = "struct ";
    private static final String END_STRUCT = "end_struct";
    private static final String CUSTOM = "custom";
    private static final String DEFINE = "#define";
    private static final String BIT = "bit";
    private static final String ROM_RAIDER_XML_TEMPLATE = "rusefi_template.xml";
    private static final String ROM_RAIDER_XML_OUTPUT = "rusefi.xml";
    private static final String ENGINE_CONFIGURATION_GENERATED_STRUCTURES_H = "engine_configuration_generated_structures.h";

    public static StringBuilder settingContextHelp = new StringBuilder();

    public static void main(String[] args) throws IOException {
        if (args.length != 4 && args.length != 5) {
            System.out.println("Please specify path to '" + INPUT_FILE_NAME + "' file, path to " + TS_FILE_INPUT_NAME +
                    ", destination folder and [optional] file name of custom prepend file");
            return;
        }

        String definitionInputPath = args[0];
        String tsPath = args[1];
        String headerDestinationFolder = args[2];
        String javaConsolePath = args[3];
        String prependFile = args.length == 5 ? args[4] : null;
        String fullFileName = definitionInputPath + File.separator + INPUT_FILE_NAME;
        System.out.println("Reading from " + fullFileName);
        String destCHeader = headerDestinationFolder + File.separator + ENGINE_CONFIGURATION_GENERATED_STRUCTURES_H;
        System.out.println("Writing C header to " + destCHeader);

        if (prependFile != null)
            readPrependValues(prependFile);

        BufferedWriter cHeader = new BufferedWriter(new FileWriter(destCHeader));

        BufferedReader definitionReader = new BufferedReader(new FileReader(fullFileName));

        CharArrayWriter tsWriter = new CharArrayWriter();

        CharArrayWriter javaFieldsWriter = new CharArrayWriter();


        ReaderState state = new ReaderState();

        ConfigurationConsumer cHeaderConsumer = new CHeaderConsumer(cHeader);
        ConfigurationConsumer tsProjectConsumer = new TSProjectConsumer(tsWriter, tsPath, state);
        ConfigurationConsumer javaFieldsConsumer = new JavaFieldsConsumer(javaFieldsWriter, state, javaConsolePath);

        processFile(state, definitionReader, cHeaderConsumer, tsProjectConsumer, javaFieldsConsumer);

        state.ensureEmptyAfterProcessing();

        cHeader.close();


        VariableRegistry.INSTANCE.writeNumericsToFile(headerDestinationFolder);

        String inputFileName = definitionInputPath + File.separator + ROM_RAIDER_XML_TEMPLATE;
        String outputFileName = javaConsolePath + File.separator + ROM_RAIDER_XML_OUTPUT;
        processTextTemplate(inputFileName, outputFileName);
    }

    private static void readPrependValues(String prependFile) throws IOException {
        BufferedReader definitionReader = new BufferedReader(new FileReader(prependFile));
        String line;
        while ((line = definitionReader.readLine()) != null) {
            line = trimLine(line);
            /**
             * we should ignore empty lines and comments
             */
            if (isEmptyDefinitionLine(line))
                continue;
            if (startsWithToken(line, DEFINE)) {
                processDefine(line.substring(DEFINE.length()).trim());
            }

        }
    }

    private static void processTextTemplate(String inputFileName, String outputFileName) throws IOException {
        System.out.println("Reading from " + inputFileName);
        System.out.println("Writing to " + outputFileName);

        VariableRegistry.INSTANCE.put("generator_message", "Generated by ConfigDefinition utility on " + new Date());

        File inputFile = new File(inputFileName);

        File outputFile = new File(outputFileName);
        BufferedReader fr = new BufferedReader(new FileReader(inputFile));
        FileWriter fw = new FileWriter(outputFile);

        String line;
        while ((line = fr.readLine()) != null) {
            line = VariableRegistry.INSTANCE.applyVariables(line);
            fw.write(line + ConfigDefinition.EOL);
        }
        fw.close();
    }

    private static void processFile(ReaderState state, BufferedReader definitionReader,
                                    ConfigurationConsumer cHeaderConsumer,
                                    ConfigurationConsumer tsProjectConsumer,
                                    ConfigurationConsumer javaFieldsConcumer) throws IOException {
        String line;

        cHeaderConsumer.startFile();

        while ((line = definitionReader.readLine()) != null) {
            line = trimLine(line);
            /**
             * we should ignore empty lines and comments
             */
            if (isEmptyDefinitionLine(line))
                continue;

            if (line.startsWith(STRUCT)) {
                handleStartStructure(state, line.substring(STRUCT.length()), true);
            } else if (line.startsWith(STRUCT_NO_PREFIX)) {
                handleStartStructure(state, line.substring(STRUCT_NO_PREFIX.length()), false);
            } else if (line.startsWith(END_STRUCT)) {
                handleEndStruct(state, cHeaderConsumer, tsProjectConsumer, javaFieldsConcumer);
            } else if (line.startsWith(BIT)) {
                handleBitLine(state, line);

            } else if (startsWithToken(line, CUSTOM)) {
                handleCustomLine(state, line);

            } else if (startsWithToken(line, DEFINE)) {
                /**
                 * for example
                 * #define CLT_CURVE_SIZE 16
                 */
                processDefine(line.substring(DEFINE.length()).trim());
            } else {
                processField(state, line);
            }
        }
        cHeaderConsumer.endFile();
        tsProjectConsumer.endFile();
        javaFieldsConcumer.endFile();
    }

    private static boolean isEmptyDefinitionLine(String line) {
        return line.length() == 0 || line.startsWith("!");
    }

    private static String trimLine(String line) {
        line = line.trim();
        line = line.replaceAll("\\s+", " ");
        return line;
    }

    private static void handleCustomLine(ReaderState state, String line) {
        line = line.substring(CUSTOM.length() + 1).trim();
        int index = line.indexOf(' ');
        String name = line.substring(0, index);
        line = line.substring(index).trim();
        index = line.indexOf(' ');
        String customSize = line.substring(0, index);

        String tunerStudioLine = line.substring(index).trim();
        tunerStudioLine = VariableRegistry.INSTANCE.applyVariables(tunerStudioLine);
        int size;
        try {
            size = Integer.parseInt(customSize);
        } catch (NumberFormatException e) {
            throw new IllegalStateException("Size in " + line);
        }
        state.tsCustomSize.put(name, size);
        state.tsCustomLine.put(name, tunerStudioLine);
    }

    private static void handleBitLine(ReaderState state, String line) {
        line = line.substring(BIT.length() + 1).trim();

        String bitName;
        String comment;
        if (!line.contains(";")) {
            bitName = line;
            comment = "";
        } else {
            int index = line.indexOf(";");
            bitName = line.substring(0, index);
            comment = line.substring(index + 1);
        }

        ConfigField bitField = new ConfigField(state, bitName, comment, true, null, null, 0, null, false);
        state.stack.peek().addBoth(bitField);
    }

    private static boolean startsWithToken(String line, String token) {
        return line.startsWith(token + " ") || line.startsWith(token + "\t");
    }

    private static void handleStartStructure(ReaderState state, String line, boolean withPrefix) {
        String name;
        String comment;
        if (line.contains(" ")) {
            int index = line.indexOf(' ');
            name = line.substring(0, index);
            comment = line.substring(index + 1).trim();
        } else {
            name = line;
            comment = null;
        }
        ConfigStructure structure = new ConfigStructure(name, comment, withPrefix);
        state.stack.push(structure);
        System.out.println("Starting structure " + structure.getName());
    }

    private static void handleEndStruct(ReaderState state, ConfigurationConsumer cHeaderConsumer,
                                        ConfigurationConsumer tsProjectConsumer,
                                        ConfigurationConsumer javaFieldsConcumer) throws IOException {
        if (state.stack.isEmpty())
            throw new IllegalStateException("Unexpected end_struct");
        ConfigStructure structure = state.stack.pop();
        System.out.println("Ending structure " + structure.getName());
        structure.addAlignmentFill(state);

        state.structures.put(structure.getName(), structure);

        cHeaderConsumer.handleEndStruct(structure);
        tsProjectConsumer.handleEndStruct(structure);
        javaFieldsConcumer.handleEndStruct(structure);

    }

    private static void processField(ReaderState state, String line) {

        ConfigField cf = ConfigField.parse(state, line);
        if (cf == null)
            throw new IllegalStateException("Cannot parse line [" + line + "]");

        if (state.stack.isEmpty())
            throw new IllegalStateException(cf.name + ": Not enclosed in a struct");
        ConfigStructure structure = state.stack.peek();

        if (cf.isIterate) {
            structure.addC(cf);
            for (int i = 1; i <= cf.arraySize; i++) {
                ConfigField element = new ConfigField(state, cf.name + i, cf.comment, false, null,
                        cf.type, 1, cf.tsInfo, false);
                structure.addTs(element);
            }
        } else {
            structure.addBoth(cf);
        }
    }

    public static String getComment(String comment, int currentOffset) {
        return "\t/**" + EOL + packComment(comment, "\t") + "\t * offset " + currentOffset + EOL + "\t */" + EOL;
    }

    public static String packComment(String comment, String linePrefix) {
        if (comment == null)
            return "";
        if (comment.trim().isEmpty())
            return "";
        String result = "";
        for (String line : comment.split("\\\\n")) {
            result += linePrefix + " * " + line + EOL;
        }
        return result;
    }

    public static int getSize(String s) {
        if (VariableRegistry.INSTANCE.intValues.containsKey(s)) {
            return VariableRegistry.INSTANCE.intValues.get(s);
        }
        return Integer.parseInt(s);
    }

    private static void processDefine(String line) {
        int index = line.indexOf(' ');
        String name;
        if (index == -1) {
            name = line;
            line = "";
        } else {
            name = line.substring(0, index);
            line = line.substring(index).trim();
        }
        VariableRegistry.INSTANCE.register(name, line);
    }
}
