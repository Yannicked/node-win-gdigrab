#include <node.h>
#include <node_buffer.h>
#include <windows.h>
#include <memory>

namespace gdi {
	using v8::FunctionCallbackInfo;
	using v8::Isolate;
	using v8::Local;
	using v8::MaybeLocal;
	using v8::Object;
	using v8::String;
	using v8::Integer;
	using v8::Value;
	using v8::Context;
	using v8::Function;
	using v8::Handle;

	HDC hScreen;
	HDC hdcMem;
	HBITMAP hBitmap;
	int ScreenX;
	int ScreenY;
	int OffsetX = 0;
	int OffsetY = 0;
	int ScaleX;
	int ScaleY;
	
	void grab(const FunctionCallbackInfo<Value>& args) {
		/*
		int horizontal_resolution = args[0]->IntegerValue();
		int vertical_resolution = args[1]->IntegerValue();
		*/
		Isolate* isolate = args.GetIsolate();
		
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
		
		char* ScreenData = (char*)malloc(4 * ScaleX * ScaleY);
		
		Local<Object> slowBuffer;
		
		GetDIBits(hdcMem, hBitmap, 0, ScaleY, ScreenData, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
		node::Buffer::New(isolate, ScreenData, 4 * ScaleX * ScaleY).ToLocal(&slowBuffer);
		
		args.GetReturnValue().Set(slowBuffer);
		/*Local<Object> globalObj = isolate->GetCurrentContext()->Global();
		Local<Function> bufferConstructor = Local<Function>::Cast(globalObj);
		Handle<Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(sizeof(ScreenData)), v8::Integer::New(0) };
		Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);*/
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
		NODE_SET_METHOD(exports, "grab", grab);
		NODE_SET_METHOD(exports, "create", create);
		NODE_SET_METHOD(exports, "destroy", destroy);
	}

	NODE_MODULE(screenres, init);
}