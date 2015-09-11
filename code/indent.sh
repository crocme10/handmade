#!/bin/bash

# -nbad     /* don't force blank lines on comments */
# -bap      /* forces blank lines after procedure */
# -bbo      /* brake long lines before boolean operator */
# -bl -bli0 /* braces on separate line with 0 indent */
# -bls      /* braces in struct in separate line */
# -blf      /* braces in function in separate line */
# /* -c33 -cd33 -cp33 */ /* comments to the right of if / else in column 33 */
# /* -ce */
# -cdb      /* comment delimiter on separate line
# -cli4     /* case indented 4 to the right of the enclosing brace */
# -cbi0     /* braces following a case are with 0 indent */
# -di0      /* don't force alignment of variables */
# -nbfde -nbfda /* don't force newline in function arguments */
# -fc1      /* format comments in the first column */
# -fca      /* do not disable all formatting of comments */
# -hnl      /* honor new lines */
# -i8       /* indentation level */
# -ip8      /* indentation level for type declaration */
# -il8      /* indentation level for labels */
# -l110     /* line length */
# -lp       /* don't line function arguments on multiline and indent newline by 4 */
# -pcs      /* forces a space between name of procedure and ( */
# -cs       /* forces a space between cast type and ( */
# -bs       /* sizeof */
# -saf -sai -saw /* spaces in for, if, and while */
# -nbc      /* don't force newline in declaration of multiple variables */
# -prs      /* space after parenthesis */
# -psl      /* type of a procedure on the line before its name */
# -nsob     /* don't swallow optional blank lines */
# -nss      /* don't force space before semicolon in for loops */
# -ts4      /* tabs are the equivalent of 4 spaces */

for file in *.c; do
    [ -e "$file" ] || continue
    echo "indenting $file"
    $HOME/dev/bin/indent -nbad -bap -bbo -br -bls -blf -cli4 -cbi0 -di0 -nbfde -nbfda -fc1 -fca -hnl -i4 -ip4 -il4 -l100 -lp -pcs -cs -bs -saf -sai -saw -nbc -psl -nsob -nss -ts4 "$file"
    sed -e $'s/\t/    /g' "$file" > "$file.bak"
    mv "$file.bak" "$file"
    sed -e 'N;s/}\n.*break/} break/' "$file" > "$file.bak"
    mv "$file.bak" "$file"
done

