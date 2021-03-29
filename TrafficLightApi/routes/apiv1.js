const express = require('express');
const router  = express.Router();

const test    = require("../controller/test");
const auth    = require("../controller/auth");
const wemos   = require("../controller/wemos");

router.post('/login', auth.getToken);
router.get('/', function (req, res) {
    res.send('hello world');
  });
// router.get('/test', auth.validateToken, test.test);
router.post('/event', wemos.postEvent);




module.exports = router;