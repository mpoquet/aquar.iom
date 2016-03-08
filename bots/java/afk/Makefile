compile: bots/Afk.java
	@mkdir -p bin
	javac -d bin -sourcepath . bots/Afk.java

run: compile
	java -Djava.library.path=. -cp bin bots.Afk ::1 4242
