use std::{error::Error, sync::Arc};
use wgpu_state::State;
use winit::{
    application::ApplicationHandler, dpi::PhysicalSize, event::WindowEvent, event_loop::EventLoop,
    window::Window,
};

mod wgpu_state;

#[derive(Default)]
pub struct Application<'window> {
    window: Option<Arc<Window>>,
    wgpu_state: Option<wgpu_state::State<'window>>,
}

impl<'window> ApplicationHandler for Application<'window> {
    fn new_events(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        cause: winit::event::StartCause,
    ) {
        let _ = (event_loop, cause);
    }

    fn user_event(&mut self, event_loop: &winit::event_loop::ActiveEventLoop, event: ()) {
        let _ = (event_loop, event);
    }

    fn device_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        device_id: winit::event::DeviceId,
        event: winit::event::DeviceEvent,
    ) {
        let _ = (event_loop, device_id, event);
    }

    fn about_to_wait(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }

    fn suspended(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }

    fn exiting(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }

    fn memory_warning(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let _ = event_loop;
    }

    fn resumed(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        if !self.window.is_none() {
            return;
        }

        let window_size = PhysicalSize::new(1280, 720);

        let window = Arc::new(
            match event_loop
                .create_window(Window::default_attributes().with_inner_size(window_size))
            {
                Ok(x) => x,
                Err(err) => {
                    panic!("ERR create window {}", err);
                }
            },
        );

        self.window = Some(window.clone());
        self.wgpu_state = Some(State::new(window.clone()));
    }

    fn window_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        window_id: winit::window::WindowId,
        event: winit::event::WindowEvent,
    ) {
        if window_id != self.window.as_ref().unwrap().id() {
            return;
        }

        match event {
            WindowEvent::Resized(new_size) => {
                self.wgpu_state.as_mut().unwrap().resize(new_size);
            }
            WindowEvent::RedrawRequested => {
                self.window.as_ref().unwrap().request_redraw();
                self.wgpu_state.as_mut().unwrap().update();

                match self.wgpu_state.as_mut().unwrap().render() {
                    Ok(_) => {}
                    Err(wgpu::SurfaceError::Lost | wgpu::SurfaceError::Outdated) => {
                        let size = self.wgpu_state.as_ref().unwrap().size;
                        self.wgpu_state.as_mut().unwrap().resize(size);
                    }
                    Err(wgpu::SurfaceError::OutOfMemory) => {
                        event_loop.exit();
                        println!("ERR out of memory");
                    }
                    Err(wgpu::SurfaceError::Timeout) => {
                        println!("Surface timeout");
                    }
                }
            }
            WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            _ => (),
        }
    }
}

fn main() -> Result<(), Box<dyn Error>> {
    println!("Hello");

    let event_loop = EventLoop::new()?;
    event_loop.set_control_flow(winit::event_loop::ControlFlow::Poll);
    let mut app = Application::default();
    event_loop.run_app(&mut app)?;

    println!("Bye");

    Ok(())
}
