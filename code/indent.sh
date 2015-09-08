#!/bin/sh

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
# -i4       /* indentation level */
# -ip4      /* indentation level for type declaration */
# -il4      /* indentation level for labels */
# -l110     /* line length */
##### -nlp -ci4 /* don't line function arguments on multiline and indent newline by 4 */
# -pcs      /* forces a space between name of procedure and ( */
# -cs       /* forces a space between cast type and ( */
# -bs       /* sizeof */
# -saf -sai -saw /* spaces in for, if, and while */
# -nbc      /* don't force newline in declaration of multiple variables */
# -prs      /* space after parenthesis */
# -psl      /* type of a procedure on the line before its name */
# -nsob     /* don't swallow optional blank lines */
# -nss      /* don't force space before semicolon in for loops */
# -ts8

# $1 directory
foo ()
{
  # get all the headers ... find ./ -name '*.h'
  # extract all the type definition ... xargs egrep '^typedef.*_t;'
  # extract the name of the type (last column) awk '{print $(NF)}'
  # remove the trailing ; tr -d ";"
  # merge all the types onto one line tr '\012\015' ' '
  # remove the trailing space sed 's/ *$//g'
  # replace all the space characters with a -T sed 's/ / -T /g'`
  #for file in `find $1 -name '*.c' -o -name '*.h'`; do
  file="main.c"
  echo "indenting $file"
  $HOME/dev/bin/indent -nbad -bap -bbo -bl -bli0 -bls -blf -cli4 -cbi0 -di0 -nbfde -nbfda -fc1 -fca -hnl -i8 -ip8 -il8 -l150 -lp -pcs -cs -bs -saf -sai -saw -nbc -psl -nsob -nss -ts8 $file
    # GNU $HOME/dev/bin/indent $file
    # LINUX $HOME/dev/bin/indent -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8 -il1 $file

  #done
}

foo .
