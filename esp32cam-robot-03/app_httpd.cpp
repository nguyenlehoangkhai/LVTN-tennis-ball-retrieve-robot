/*
  ESP32CAM Robot Car
  app_httpd.cpp (requires esp32cam-robot.ino)
  Based upon Espressif ESP32CAM Examples
  Uses TBA6612FNG H-Bridge Controller
  
  DroneBot Workshop 2021
  https://dronebotworkshop.com
*/

#include "dl_lib_matrix3d.h"
#include <esp32-hal-ledc.h>
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "Arduino.h"

// TB6612FNG H-Bridge Connections (both PWM inputs driven by GPIO 2)
#define MTR_PWM   2
#define LEFT_M0     12
#define LEFT_M1     13
#define RIGHT_M0    15
#define RIGHT_M1    14

// Define Speed variables
int speed = 255;
int noStop = 0;

//Setting Motor PWM properties
const int freq = 2000;
const int motorPWMChannnel = 8;
const int lresolution = 8;

volatile unsigned int  motor_speed   = 200;
volatile unsigned long previous_time = 0;
volatile unsigned long move_interval = 250;

// Placeholder for functions
void robot_setup();
void robot_stop();
void robot_fwd();
void robot_back();
void robot_left();
void robot_right();
uint8_t robo = 0;


typedef struct {
  httpd_req_t *req;
  size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len) {
  jpg_chunking_t *j = (jpg_chunking_t *)arg;
  if (!index) {
    j->len = 0;
  }
  if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
    return 0;
  }
  j->len += len;
  return len;
}

static esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  int64_t fr_start = esp_timer_get_time();

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

  size_t out_len, out_width, out_height;
  uint8_t * out_buf;
  bool s;
  {
    size_t fb_len = 0;
    if (fb->format == PIXFORMAT_JPEG) {
      fb_len = fb->len;
      res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    } else {
      jpg_chunking_t jchunk = {req, 0};
      res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
      httpd_resp_send_chunk(req, NULL, 0);
      fb_len = jchunk.len;
    }
    esp_camera_fb_return(fb);
    int64_t fr_end = esp_timer_get_time();
    Serial.printf("JPG: %uB %ums\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
    return res;
  }

  dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  if (!image_matrix) {
    esp_camera_fb_return(fb);
    Serial.println("dl_matrix3du_alloc failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  out_buf = image_matrix->item;
  out_len = fb->width * fb->height * 3;
  out_width = fb->width;
  out_height = fb->height;

  s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
  esp_camera_fb_return(fb);
  if (!s) {
    dl_matrix3du_free(image_matrix);
    Serial.println("to rgb888 failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  jpg_chunking_t jchunk = {req, 0};
  s = fmt2jpg_cb(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);
  dl_matrix3du_free(image_matrix);
  if (!s) {
    Serial.println("JPEG compression failed");
    return ESP_FAIL;
  }

  int64_t fr_end = esp_timer_get_time();
  return res;
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];
  dl_matrix3du_t *image_matrix = NULL;

  static int64_t last_frame = 0;
  if (!last_frame) {
    last_frame = esp_timer_get_time();
  }

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      {
        if (fb->format != PIXFORMAT_JPEG) {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted) {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) {
      break;
    }
    int64_t fr_end = esp_timer_get_time();
    int64_t frame_time = fr_end - last_frame;
    last_frame = fr_end;
    frame_time /= 1000;
    Serial.printf("MJPG: %uB %ums (%.1ffps)\n",
                  (uint32_t)(_jpg_buf_len),
                  (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time
                 );
  }

  last_frame = 0;
  return res;
}

enum state {fwd, rev, stp};
state actstate = stp;

static esp_err_t cmd_handler(httpd_req_t *req)
{
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};
  char value[32] = {0,};

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if (!buf) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
          httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
      } else {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int val = atoi(value);
  sensor_t * s = esp_camera_sensor_get();
  int res = 0;

// Look at values within URL to determine function
  if (!strcmp(variable, "framesize"))
  {
    Serial.println("framesize");
    if (s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
  }
  else if (!strcmp(variable, "quality"))
  {
    Serial.println("quality");
    res = s->set_quality(s, val);
  }
  else if (!strcmp(variable, "flash"))
  {
    ledcWrite(7, val);
  }
  else if (!strcmp(variable, "flashoff"))
  {
    ledcWrite(7, val);
  }
  else if (!strcmp(variable, "speed"))
  {
    if      (val > 255) val = 255;
    else if (val <   0) val = 0;
    speed = val;
    ledcWrite(8, speed);
  }
  else if (!strcmp(variable, "nostop"))
  {
    noStop = val;
  }
  else if (!strcmp(variable, "car")) {
    if (val == 1) {
      Serial.println("Forward");
      robot_fwd();
      robo = 1;
    }
    else if (val == 2) {
      Serial.println("TurnLeft");
      robot_left();
      robo = 1;
    }
    else if (val == 3) {
      Serial.println("Stop");
      robot_stop();
    }
    else if (val == 4) {
      Serial.println("TurnRight");
      robot_right();
      robo = 1;
    }
    else if (val == 5) {
      Serial.println("Backward");
      robot_back();
      robo = 1;
    }
    if (noStop != 1)
    {

    }
  }
  else
  {
    Serial.println("variable");
    res = -1;
  }

  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req) {
  static char json_response[1024];

  sensor_t * s = esp_camera_sensor_get();
  char * p = json_response;
  *p++ = '{';

  p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
  p += sprintf(p, "\"quality\":%u,", s->status.quality);
  *p++ = '}';
  *p++ = 0;
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json_response, strlen(json_response));
}

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<<!doctype html>
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width,initial-scale=1">
        <title>Tennis Retriever Robot - Nguyen Le Hoang Khai - HCMUT</title>
        <style>
    body{font-family:Arial, Helvetica,sans-serif; background:#181818; color:#efefef; font-size:16px}
    h2{font-size:18px}section.main{display:flex}#menu,section.main{flex-direction:column}
    #menu{display:none;flex-wrap:nowrap;min-width:340px;background:#363636;padding:8px;border-radius:4px;margin-top:-10px;margin-right:10px}
    #content{display:flex;flex-wrap:wrap;align-items:stretch}
    figure{padding:0;margin:0;-webkit-margin-before:0;margin-block-start:0;-webkit-margin-after:0;margin-block-end:0;-webkit-margin-start:0;margin-inline-start:0;-webkit-margin-end:0;margin-inline-end:0}
    figure img{display:block;width:100%;height:auto;border-radius:4px;margin-top:8px}
    @media (min-width:800px) and (orientation:landscape){#content{display:flex;flex-wrap:nowrap;align-items:stretch}
    figure img{display:block;max-width:100%;max-height:calc(100vh - 40px);width:auto;height:auto}
    figure{padding:0;margin:0;-webkit-margin-before:0;margin-block-start:0;-webkit-margin-after:0;margin-block-end:0;-webkit-margin-start:0;margin-inline-start:0;-webkit-margin-end:0;margin-inline-end:0}}
    
    section #buttons{display:flex;flex-wrap:nowrap;justify-content:space-between}
          #nav-toggle{cursor:pointer;display:block}
          #nav-toggle-cb{outline:0;opacity:0;width:0;height:0}
          #nav-toggle-cb:checked+#menu{display:flex}
          .input-group{display:flex;flex-wrap:nowrap;line-height:22px;margin:5px 0}
          .input-group>label{display:inline-block;padding-right:10px;min-width:47%}
          .input-group input,
          .input-group select{flex-grow:1}.range-max,.range-min{display:inline-block;padding:0 5px}
      button{display:block;margin:5px;padding:5px 12px;border:0;line-height:28px;cursor:pointer;color:#fff;background:#035806;border-radius:5px;font-size:16px;outline:0;width:100px}
           .button2{background-color:#008cba;width:110px}
           .button3{background-color:#f44336;width:100px}
           .button4{background-color:#e7e7e7;color:#000;width:120px}
           .button5{background-color:#555;width:100px}
           .button6{visibility:hidden;width:100px}button:hover{background:#ff494d}button:active{background:#f21c21}
      button.disabled{cursor:default;background:#a0a0a0}
          
    input[type=range]{-webkit-appearance:none;width:100%;height:22px;background:#363636;cursor:pointer;margin:0}
    input[type=range]:focus{outline:0}input[type=range]::-webkit-slider-runnable-track{width:100%;height:2px;cursor:pointer;background:#efefef;border-radius:0;border:0 solid #efefef}
    input[type=range]::-webkit-slider-thumb{border:1px solid rgba(0,0,30,0);height:22px;width:22px;border-radius:50px;background:#ff3034;cursor:pointer;-webkit-appearance:none;margin-top:-11.5px}
    input[type=range]:focus::-webkit-slider-runnable-track{background:#efefef}
    input[type=range]::-moz-range-track{width:100%;height:2px;cursor:pointer;background:#efefef;border-radius:0;border:0 solid #efefef}
    input[type=range]::-moz-range-thumb{border:1px solid rgba(0,0,30,0);height:22px;width:22px;border-radius:50px;background:#ff3034;cursor:pointer}
    input[type=range]::-ms-track{width:100%;height:2px;cursor:pointer;background:0 0;border-color:transparent;color:transparent}
    input[type=range]::-ms-fill-lower{background:#efefef;border:0 solid #efefef;border-radius:0}
    input[type=range]::-ms-fill-upper{background:#efefef;border:0 solid #efefef;border-radius:0}
    input[type=range]::-ms-thumb{border:1px solid rgba(0,0,30,0);height:22px;width:22px;border-radius:50px;background:#ff3034;cursor:pointer;height:2px}
    input[type=range]:focus::-ms-fill-lower{background:#efefef}
    input[type=range]:focus::-ms-fill-upper{background:#363636}
    .switch{display:block;position:relative;line-height:22px;font-size:16px;height:22px}
    .switch input{outline:0;opacity:0;width:0;height:0}
    .slider{width:50px;height:22px;border-radius:22px;cursor:pointer;background-color:grey}.slider,.slider:before{display:inline-block;transition:.4s}
    .slider:before{position:relative;content:"";border-radius:50%;height:16px;width:16px;left:4px;top:3px;background-color:#fff}
    
    input:checked+.slider{background-color:#ff3034}
    input:checked+.slider:before{-webkit-transform:translateX(26px);transform:translateX(26px)}
    select{border:1px solid #363636;font-size:14px;height:22px;outline:0;border-radius:5px}
    .image-container{position:absolute;top:50px;left:50%;margin-right:-50%;transform:translate(-50%,-50%);min-width:160px}
    .control-container{position:relative;top:400px;left:50%;margin-right:-50%;transform:translate(-50%,-50%)}
    .slider-container{position:absolute;top:750px;margin-left:45%;transform:translate(-50%,-50%)}
    .close{position:absolute;right:5px;top:5px;background:#ff3034;width:16px;height:16px;border-radius:100px;color:#fff;text-align:center;line-height:18px;cursor:pointer}
    .hidden{display:none}
    .rotate90{-webkit-transform:rotate(270deg);-moz-transform:rotate(270deg);-o-transform:rotate(270deg);-ms-transform:rotate(270deg);transform:rotate(270deg)}
           
          
    * {margin: 0; border: 0; padding: 0;}
    #logo {
        display: inline-block;
        margin: 20px; 
        float: left;
        height: 70px;
        width: auto; 
        border-radius: 0;
        position:absolute;
        top:15px;
        left:15px;
          }

    #logo2 { 
        margin: 20px; 
        float: right;
        height: 70px;
        width: auto; 
        border-radius: 0;
        position:absolute;
        top:15px;
        right :15px;
          }

      </style>

    </head>
    <body>
    
    <br>
    <br>
        <section class="main">
        <figure>
        <div id="stream-container" class="image-container">
        <div class="close" id="close-stream">×</div>
        <img id="stream" src="" class="rotate90">
        </div>
    </figure>
    <br>
    <br>
    
            
            <section id="buttons">
              <!-- <script src="https://cdnjs.cloudflare.com/ajax/libs/rxjs/4.0.6/rx.testing.js"></script> -->
                <div id="controls" class="control-container">
                  
                  <h1 align ="center">TENNIS RETRIEVER ROBOT - NLHK</h1>
                  <table>
                  <tr><a href="#"><img id="logo" src="https://upload.wikimedia.org/wikipedia/vi/c/cd/Logo-hcmut.svg"></a>
                      <a href="#"><img id="logo2" src="data:image/jpeg;base64,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"></a>
                      <td align="center"><button class="button button6" id="get-still">Image</button></td>
                      <td align="center"><button  id="toggle-stream">Start</button></td>
                      <td></td></tr>
                  
                  <tr><td></td><td align="center"><button class="button button2" id="forward" onclick="fetch(document.location.origin+'/control?var=car&val=1');">FORWARD</button></td><td></td></tr>
                  
                  <tr><td align="center"><button class="button button2" id="turnleft" onclick="fetch(document.location.origin+'/control?var=car&val=2');">LEFT</button></td>
                      <td align="center"><button class="button button4"id="stop">STOP</button></td>
                      <td align="center"><button class="button button2" id="turnright" onclick="fetch(document.location.origin+'/control?var=car&val=4');">RIGHT</button></td></tr>
                  
                  <tr>
                      <td align="center"><button class="button button4"id="retrieve">RETRIEVE</button></td>
                      <td align="center"><button class="button button2" id="backward" onclick="fetch(document.location.origin+'/control?var=car&val=5');">REVERSE</button></td>
                      <td align="center"><button class="button button4"id="auto">AUTO</button></td></tr>

                  <tr><td></td><td align="center"><button class="button button4" id="flash" onclick="fetch(document.location.origin+'/control?var=flash&val=1');">LIGHT ON</button></td><td></td></tr>
                  <tr><td></td><td align="center"><button class="button button4" id="flashoff" onclick="fetch(document.location.origin+'/control?var=flashoff&val=0');">LIGHT OFF</button></td><td></td></tr>
                  
                  </table>
               </div>
               
              <br>
              
                <!-- <div class="slidecontainer"><input type="range" min="1" max="100" value="50" class="slider" id="myRange"></div> -->
                <div id="sliders" class="slider-container">
                  <table>
                    <tr><td></td><td >Motor Speed:</td><td align="center" colspan="2"><input type="range" id="speed" min="0" max="255" value="200" onchange="try{fetch(document.location.origin+'/control?var=speed&val='+this.value);}catch(e){}"></td><td></td></tr>
                    <tr><td></td><td >Vid Quality:</td><td align="center" colspan="2"><input type="range" id="quality" min="10" max="63" value="10" onchange="try{fetch(document.location.origin+'/control?var=quality&val='+this.value);}catch(e){}"></td><td></td></tr>
                    <tr><td></td><td >Vid Size:</td><td align="center" colspan="2"><input type="range" id="framesize" min="0" max="6" value="5" onchange="try{fetch(document.location.origin+'/control?var=framesize&val='+this.value);}catch(e){}"></td></tr>
                 
                  </table>
                </div>
              
            </section>         
        </section>
        <script> document.addEventListener('DOMContentLoaded',function(){function b(B){let C;switch(B.type){case'checkbox':C=B.checked?1:0;break;case'range':case'select-one':C=B.value;break;case'button':case'submit':C='1';break;default:return;}const D=`${c}/control?var=${B.id}&val=${C}`;fetch(D).then(E=>{console.log(`request to ${D} finished, status: ${E.status}`)})}
        var c=document.location.origin;const e=B=>{B.classList.add('hidden')},f=B=>{B.classList.remove('hidden')},g=B=>{B.classList.add('disabled'),B.disabled=!0},h=B=>{B.classList.remove('disabled'),B.disabled=!1},i=(B,C,D)=>{D=!(null!=D)||D;let E;'checkbox'===B.type?(E=B.checked,C=!!C,B.checked=C):(E=B.value,B.value=C),D&&E!==C?b(B):!D&&('aec'===B.id?C?e(v):f(v):'agc'===B.id?C?(f(t),e(s)):(e(t),f(s)):'awb_gain'===B.id?C?f(x):e(x):'face_recognize'===B.id&&(C?h(n):g(n)))};
        document.querySelectorAll('.close').forEach(B=>{B.onclick=()=>{e(B.parentNode)}}),fetch(`${c}/status`).then(function(B){return B.json()}).then(function(B){document.querySelectorAll('.default-action').forEach(C=>{i(C,B[C.id],!1)})});
        const j=document.getElementById('stream'),k=document.getElementById('stream-container'),l=document.getElementById('get-still'),m=document.getElementById('toggle-stream'),n=document.getElementById('face_enroll'),o=document.getElementById('close-stream'),p=()=>{window.stop(),m.innerHTML='Start'},q=()=>{j.src=`${c+':81'}/stream`,f(k),m.innerHTML='Stop'};l.onclick=()=>{p(),j.src=`${c}/capture?_cb=${Date.now()}`,f(k)},o.onclick=()=>{p(),e(k)},m.onclick=()=>{const B='Stop'===m.innerHTML;B?p():q()},n.onclick=()=>{b(n)},document.querySelectorAll('.default-action').forEach(B=>{B.onchange=()=>b(B)});
        const r=document.getElementById('agc'),s=document.getElementById('agc_gain-group'),t=document.getElementById('gainceiling-group');r.onchange=()=>{b(r),r.checked?(f(t),e(s)):(e(t),f(s))};const u=document.getElementById('aec'),v=document.getElementById('aec_value-group');u.onchange=()=>{b(u),u.checked?e(v):f(v)};const w=document.getElementById('awb_gain'),x=document.getElementById('wb_mode-group');w.onchange=()=>{b(w),w.checked?f(x):e(x)};const y=document.getElementById('face_detect'),z=document.getElementById('face_recognize'),A=document.getElementById('framesize');A.onchange=()=>{b(A),5<A.value&&(i(y,!1),i(z,!1))},
        y.onchange=()=>{return 5<A.value?(alert('Please select CIF or lower resolution before enabling this feature!'),
        void i(y,!1)):void(b(y),!y.checked&&(g(n),i(z,!1)))},z.onchange=()=>{return 5<A.value?(alert('Please select CIF or lower resolution before enabling this feature!'),void i(z,!1)):void(b(z),z.checked?(h(n),i(y,!0)):g(n))}})</script>
    </body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

void startCameraServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t status_uri = {
        .uri       = "/status",
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t capture_uri = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
    };

   httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };
    
    Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        httpd_register_uri_handler(camera_httpd, &capture_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}

unsigned int get_speed(unsigned int sp)
{
  return map(sp, 0, 100, 0, 255);
}

void robot_setup()
{
    // Pins for Motor Controller
    pinMode(LEFT_M0,OUTPUT);
    pinMode(LEFT_M1,OUTPUT);
    pinMode(RIGHT_M0,OUTPUT);
    pinMode(RIGHT_M1,OUTPUT);
    
    // Make sure we are stopped
    robot_stop();

    // Motor uses PWM Channel 8
    ledcAttachPin(MTR_PWM, 8);
    ledcSetup(8, 2000, 8);      
    ledcWrite(8, 130);
    
}

// Motor Control Functions

void update_speed()
{  
    ledcWrite(motorPWMChannnel, get_speed(motor_speed));
    
}

void robot_stop()
{
  digitalWrite(LEFT_M0,LOW);
  digitalWrite(LEFT_M1,LOW);
  digitalWrite(RIGHT_M0,LOW);
  digitalWrite(RIGHT_M1,LOW);
}

void robot_back()
{
  digitalWrite(LEFT_M0,HIGH);
  digitalWrite(LEFT_M1,LOW);
  digitalWrite(RIGHT_M0,HIGH);
  digitalWrite(RIGHT_M1,LOW);
  move_interval=250;
  previous_time = millis();  
}

void robot_fwd()
{
  digitalWrite(LEFT_M0,LOW);
  digitalWrite(LEFT_M1,HIGH);
  digitalWrite(RIGHT_M0,LOW);
  digitalWrite(RIGHT_M1,HIGH);
  move_interval=250;
   previous_time = millis();  
}

void robot_right()
{
  digitalWrite(LEFT_M0,LOW);
  digitalWrite(LEFT_M1,LOW);
  digitalWrite(RIGHT_M0,LOW);
  digitalWrite(RIGHT_M1,HIGH);
  move_interval=100;
   previous_time = millis();
}

void robot_left()
{
  digitalWrite(LEFT_M0,LOW);
  digitalWrite(LEFT_M1,HIGH);
  digitalWrite(RIGHT_M0,LOW);
  digitalWrite(RIGHT_M1,LOW);
  move_interval=100;
   previous_time = millis();
}
