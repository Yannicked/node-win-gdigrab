#include <node.h>
#include <memory>
#include <uv.h>
#include <node_buffer.h>

namespace gdi {
	using namespace v8;

	HDC hScreen;
	HDC hdcMem;
	HBITMAP hBitmap;
	int ScreenX;
	int ScreenY;
	int OffsetX = 0;
	int OffsetY = 0;
	int ScaleX;
	int ScaleY;
	
	struct Work {
	  uv_work_t request;
	  Persistent<Function> callback;
	  
	  char* returndata;
	  int returnsize;
	};
	
	void workAsync(uv_work_t *req) {
		Work *work = static_cast<Work *>(req->data);
		
		HGDIOBJ hOld = SelectObject(hdcMem, hBitmap);
		SetStretchBltMode(hdcMem,HALFTONE);
		// BitBlt(hdcMem, 0, 0, ScreenX, ScreenY, hScreen, OffsetX, OffsetY, SRCCOPY);
		StretchBlt(hdcMem, 0, 0, ScaleX, ScaleY, hScreen, OffsetX, OffsetY, ScreenX, ScreenY, SRCCOPY);
		SelectObject(hdcMem, hOld);

		BITMAPINFO bmi = {0};
		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biWidth = ScaleX;
		bmi.bmiHeader.biHeight = -ScaleY;
		
		work->returnsize = 4 * ScaleX * ScaleY;
		work->returndata = (char*)malloc(work->returnsize);
		
		GetDIBits(hdcMem, hBitmap, 0, ScaleY, work->returndata, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
	}
	
	void workAsyncComplete(uv_work_t *req, int status) {
		Isolate * isolate = Isolate::GetCurrent();
		HandleScope handleScope(isolate);
		Work *work = static_cast<Work *>(req->data);
		
		Local<Uint8Array> rgb = Uint8Array::New(ArrayBuffer::New(isolate, work->returnsize/4*3),0,work->returnsize/4*3);
		uint8_t *rgbptr = (uint8_t*)rgb->Buffer()->GetContents().Data();
		
		for(unsigned int i = 0; i<work->returnsize; i+=4) {			
			unsigned int n = i/4*3;
			
			rgbptr[n] = work->returndata[i+2];
			rgbptr[n+1] = work->returndata[i+1];
			rgbptr[n+2] = work->returndata[i];
		}
		
		//Local<Object> slowBuffer;
		//node::Buffer::New(isolate, work->returndata, work->returnsize).ToLocal(&slowBuffer);
		
		Handle<Value> argv[] = {rgb};
		
		Local<Function>::New(isolate, work->callback)->
		Call(isolate->GetCurrentContext()->Global(), 1, argv);
		work->callback.Reset();
		
		delete work;
	}
	
	void grabAsync(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		Work * work = new Work();
		work->request.data = work;
		
		Local<Function> callback = Local<Function>::Cast(args[0]);
		work->callback.Reset(isolate, callback);
		uv_queue_work(uv_default_loop(),&work->request,
			workAsync,workAsyncComplete);

		args.GetReturnValue().Set(Undefined(isolate));
	}
	
	void create(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		
		hScreen = GetDC(0);
		hdcMem = CreateCompatibleDC(hScreen);
		ScreenX = GetDeviceCaps(hScreen, HORZRES);
		ScreenY = GetDeviceCaps(hScreen, VERTRES);
		
		if (args.Length() > 0) {
			Local<Object> settingsObj = Local<Object>::Cast(args[0]);
			
			if (settingsObj->Has(String::NewFromUtf8(isolate, "OffsetX"))) {
				OffsetX = settingsObj->Get(String::NewFromUtf8(isolate, "OffsetX"))->Int32Value();
			}
			
			if (settingsObj->Has(String::NewFromUtf8(isolate, "OffsetY"))) {
				OffsetY = settingsObj->Get(String::NewFromUtf8(isolate, "OffsetY"))->Int32Value();
			}
			
			if (settingsObj->Has(String::NewFromUtf8(isolate, "ScaleX"))) {
				ScaleX = settingsObj->Get(String::NewFromUtf8(isolate, "ScaleX"))->Int32Value();
			}
			
			if (settingsObj->Has(String::NewFromUtf8(isolate, "ScaleY"))) {
				ScaleY = settingsObj->Get(String::NewFromUtf8(isolate, "ScaleY"))->Int32Value();
			}
		} else {
			ScaleY = ScreenY;
			ScaleX = ScreenX;
		}
		hBitmap = CreateCompatibleBitmap(hScreen, ScreenX, ScreenY);
	}
	
	void destroy(const FunctionCallbackInfo<Value>& args) {
		ReleaseDC(GetDesktopWindow(),hScreen);
		DeleteDC(hdcMem);
		DeleteObject(hBitmap);
	}
	
	void init(Local<Object> exports) {
		NODE_SET_METHOD(exports, "grab", grabAsync);
		NODE_SET_METHOD(exports, "create", create);
		NODE_SET_METHOD(exports, "destroy", destroy);
	}

	NODE_MODULE(screenres, init);
}