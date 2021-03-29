var Module = module.exports;
Module.getToken  = getToken;
Module.validateToken = validateToken;
Module.getToken = getToken;

const VALID_TOKEN = "FKJBIH#WH(UIBFIUWB@*FY*BF@*OIFB";

function getToken(req, res) {
    const user     = req.body.user;
    const password = req.body.password;

    console.log("getToken: " + user + ", " + password);

    if ( user == "Lucas" && password == "123hihihi") {
        res.status(200).json({token: VALID_TOKEN});

    } else {
        res.status(401).json({status: "sorry!"});
    }
}

function validateToken(req, res, next) {
    const token = req.query.token;
    
    if ( token == VALID_TOKEN ) {
        next();
    } else {
        res.status(401).json({status: "Not authorized!"});
    }
}