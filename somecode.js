  function gethex(r,g,b){ 
          r = r.toString(16); 
          g = g.toString(16); 
          b = b.toString(16); 
 
          // 补0 
          r.length==1? r = '0' + r : ''; 
          g.length==1? g = '0' + g : ''; 
          b.length==1? b = '0' + b : ''; 
 
          var hex = r + g + b; 
 
          // 简化处理,如 FFEEDD 可以写为 FED 
          if(r.slice(0,1)==r.slice(1,1) && g.slice(0,1)==g.slice(1,1) && b.slice(0,1)==b.slice(1,1)){ 
            hex = r.slice(0,1) + g.slice(0,1) + b.slice(0,1); 
          } 
 
          return hex; 
        } 

var canvas2 = document.getElementById('canvas2');
var ctxt2 = canvas2.getContext('2d');
var img1 = new Image;
img1.onload = function(){
    ctxt2.drawImage(img1,0,0, 200,200);
}
img1.src = 'assets/8-13.jpg';

var canvas1 = document.getElementById('canvas1');
var ctxt1 = canvas1.getContext('2d');
var img2 = new Image;
img2.onload = function(){
    ctxt1.drawImage(img2,0,0, 200,200);
}
img2.src = 'assets/8-14.jpg';

var canvas = document.getElementById('canvas3'); 
if (canvas.getContext){ 
var context = canvas.getContext('2d'); 



var color = [];

var s = '';


var data= ctxt1.getImageData(0,0,480,480).data;

s+='[';
s+=data[0];

s+=",";
s+=data[1];

s+=",";
s+=data[2];

s+=",";
s+=data[3];

s+=']';

var imgdata = context.getImageData(480,480, 1, 1); 
var pixels = imgdata.data; 
 
// 遍历每个像素并对 RGB 值进行取反
for (var x=0, n=data.length; x<n; x+= 4){ 
     pixels[i] = data[x]; 
      pixels[i+1] = data[x+1]; 
      pixels[i+2] = data[x+2]; 
} 


var hex = gethex(data[0],data[1],data[2]);
color.push('#'+hex);
 context.putImageData(imgdata,480,480); 
// console.log('%c 0 ', 'background: #FFFFFF; color: #'+hex);

//  console.log(s);


var canvas = document.getElementById('canvas3'); 

var context = canvas.getContext('2d'); 

var imgdata= ctxt2.getImageData(0,0,480,480);
var pixels = imgdata.data; 
for (var i=0, n=pixels.length; i<n; i+= 4){ 

for (var j=0; i<n; i+=4){ 
     pixels[i] = 255-pixels[i]; 
      pixels[i+1] = 255-pixels[i+1]; 
      pixels[i+2] = 255-pixels[i+2]; 
} 
}

 context.putImageData(imgdata,0,0); 



var canvas = document.getElementById('canvas3'); 

var context = canvas.getContext('2d'); 
var imgdata2= ctxt2.getImageData(0,0,480,480);
var imgdata3= ctxt1.getImageData(0,0,480,480);

var imgdata= context.getImageData(0,0,480,480);
var pixels = imgdata.data; 
for (var i=0, n=imgdata3.data.length; i<n; i+= 4){ 

if(imgdata3.data[i]!=imgdata2.data[i]&&imgdata3.data[i+1]!=imgdata2.data[i+1]&&imgdata3.data[i+2]!=imgdata2.data[i+2]){
pixels[i]=imgdata2.data[i];pixels[i+1]=imgdata2.data[i+1];pixels[i+2]=imgdata2.data[i+2];
}else{pixels[i]=255;pixels[i+1]=255;pixels[i+2]=255;}
}

 context.putImageData(imgdata,0,0); 




