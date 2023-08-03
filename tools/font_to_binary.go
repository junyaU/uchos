package main

import (
	"bufio"
	"flag"
	"fmt"
	"os"
	"regexp"
	"strings"
)

func fontToBinary(outputName, inputName string)(error) {
	input,err := os.Open(inputName)
	if err != nil {
		return  err
	}
	defer input.Close()

	output, err := os.Create(outputName)
	if err != nil {
		return err
	}
	defer output.Close()

	writer := bufio.NewWriter(output)

	scanner := bufio.NewScanner(input)
	for scanner.Scan() {
		line := scanner.Text()

		if compilePattern, _ := regexp.MatchString(`^[.@]{8}$`, line); !compilePattern {
			continue
		}

		compiledLine := strings.Replace(line, ".", "0", -1)
		compiledLine = strings.Replace(compiledLine, "@", "1", -1)
		
		var byteVal byte
		for i := 0; i < len(compiledLine); i++ {
			byteVal <<= 1
			if compiledLine[i] == '1' {
				byteVal |= 1
			}
		}

		writer.WriteByte(byteVal)
	}

	if err := scanner.Err(); err != nil {
		return  err	
	}

	return writer.Flush()
}	

func main() {
	flagSet := flag.NewFlagSet(os.Args[0], flag.ExitOnError)
	output := flagSet.String("o", "", "output file")
	input := flagSet.String("i", "", "input file")

	if err := flagSet.Parse(os.Args[1:]); err != nil {
		fmt.Printf("Error: %s\n", err)
		os.Exit(1)
	}

	 if err := fontToBinary(*output, *input); err != nil {
		fmt.Printf("Error: %s\n", err)
		os.Exit(1)
	}
}