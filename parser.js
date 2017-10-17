// bnf http://guangzhou.cs.uwm.edu/javascript/js.html
var Parser = function (code) {
    this.lexer = new Lexer(code);
    this.look = null;
};


Parser.prototype.move = function () {
    this.look = this.lexer.scan();
};

Parser.prototype.error = function (error) {
    throw new Error("near line " + this.lexer.line + " : " + error);
};

Parser.prototype.match = function (t) {
    if (this.look.value === t) this.move();
    else this.error("syntax error");
};


Parser.prototype.matchType = function (t) {
    if (this.look.type === t) this.move();
    else this.error("syntax error");
};

Parser.prototype.program = function () {
    sourceElements();
};

Parser.prototype.program = function () {
    sourceElements();
};

Parser.prototype.sourceElements = function () {
    sourceElement()
};

Parser.prototype.statements = function () {
    statement();
};


Parser.prototype.sourceElement = function () {
    functionDeclaration();
    statement();
};

Parser.prototype.functionDeclaration = function () {
    this.match("function");
    this.matchType(tokenType.id);
    this.match("(");
    this.formalParameterList();
    this.match(")");
    this.functionBody();
};

Parser.prototype.formalParameterList = function () {
    this.matchType(tokenType.id);
    if (this.look.value === ",") {
        do {
            this.matchType(tokenType.id);
        } while (this.look.value === ",")
    }
};

Parser.prototype.functionBody = function () {
    this.match("{");
    statement();
    returnStatement();
    this.match("}");
};

Parser.prototype.statement = function () {

    switch (this.look.value) {
        case "{":
            block();
        case "var":
            block();
        case "if":
            block();
        case "while":
            block();
        case "":
            block();
        // TODO
    }


};


Parser.prototype.block = function () {

};


Parser.prototype.VariableStatement = function () {

};


Parser.prototype.emptyStatement = function () {

};


Parser.prototype.ifStatement = function () {

};


Parser.prototype.iterationStatement = function () {

};


Parser.prototype.selectionStatement = function () {

};


Parser.prototype.updateStatement = function () {

};


Parser.prototype.methodCallStatement = function () {

};


Parser.prototype.functionCallStatement = function () {

};


Parser.prototype.allocationStatement = function () {

};


Parser.prototype.assignmentStatement = function () {

};



